
#include "php_mustache.hpp"



// Declarations ----------------------------------------------------------------

extern void mustache_data_from_zval(mustache::Data * node, zval * current TSRMLS_DC);
extern void mustache_data_to_zval(mustache::Data * node, zval * current TSRMLS_DC);
extern void mustache_node_to_zval(mustache::Node * node, zval * current TSRMLS_DC);

extern zend_class_entry * MustacheAST_ce_ptr;
extern zend_class_entry * MustacheCode_ce_ptr;
extern zend_class_entry * MustacheData_ce_ptr;
extern zend_class_entry * MustacheTemplate_ce_ptr;



// Class Entries  --------------------------------------------------------------

zend_class_entry * Mustache_ce_ptr;



// Object Handlers -------------------------------------------------------------

static zend_object_handlers Mustache_obj_handlers;

static void Mustache_obj_free(void *object TSRMLS_DC)
{
  try {
    php_obj_Mustache * payload = (php_obj_Mustache *)object;

    if( payload->mustache != NULL ) {
      delete payload->mustache;
    }
    
    efree(object);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}

static zend_object_value Mustache_obj_create(zend_class_entry *class_type TSRMLS_DC)
{
  zend_object_value retval;
  
  try {
    php_obj_Mustache * payload = NULL;
    zval * tmp = NULL;

    payload = (php_obj_Mustache *) emalloc(sizeof(php_obj_Mustache));
    memset(payload, 0, sizeof(php_obj_Mustache));
    payload->obj.ce = class_type;

    payload->mustache = new mustache::Mustache;
    
    // Set ini settings
    if( MUSTACHEG(default_escape_by_default) ) {
      payload->mustache->setEscapeByDefault(true);
    } else {
      payload->mustache->setEscapeByDefault(false);
    }
    if( MUSTACHEG(default_start_sequence) ) {
      payload->mustache->setStartSequence(MUSTACHEG(default_start_sequence), 0);
    }
    if( MUSTACHEG(default_stop_sequence) ) {
      payload->mustache->setStopSequence(MUSTACHEG(default_stop_sequence), 0);
    }
    
    retval.handle = zend_objects_store_put(payload, NULL, 
        (zend_objects_free_object_storage_t) Mustache_obj_free, NULL TSRMLS_CC);
    retval.handlers = &Mustache_obj_handlers;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }

  return retval;
}



// Utility ---------------------------------------------------------------------

bool mustache_parse_data_param(zval * data, mustache::Mustache * mustache, mustache::Data ** node TSRMLS_DC)
{
  php_obj_MustacheData * mdPayload = NULL;
  
  if( Z_TYPE_P(data) == IS_OBJECT ) {
    if( Z_OBJCE_P(data) == MustacheData_ce_ptr ) {
      mdPayload = (php_obj_MustacheData *) zend_object_store_get_object(data TSRMLS_CC);
      *node = mdPayload->data;
      return true;
    } else {
      mustache_data_from_zval(*node, data TSRMLS_CC);
      return true;
    }
  } else {
    mustache_data_from_zval(*node, data TSRMLS_CC);
    return true;
  }
}

bool mustache_parse_partials_param(zval * array, mustache::Mustache * mustache,
        mustache::Node::Partials * partials TSRMLS_DC)
{
  HashTable * data_hash = NULL;
  HashPosition data_pointer = NULL;
  zval **data_entry = NULL;
  long data_count = 0;
  
  int key_type = 0;
  char * key_str = NULL;
  uint key_len = 0;
  ulong key_nindex = 0;
  std::string ckey;
  
  std::string tmpl;
  mustache::Node node;
  mustache::Node * nodePtr = NULL;
  
  php_obj_MustacheTemplate * mtPayload = NULL;
  php_obj_MustacheAST * maPayload = NULL;
  
  // Ignore if not an array
  if( array == NULL || Z_TYPE_P(array) != IS_ARRAY ) {
    return false;
  }
  
  // Iterate over input data
  data_hash = HASH_OF(array);
  data_count = zend_hash_num_elements(data_hash);
  zend_hash_internal_pointer_reset_ex(data_hash, &data_pointer);
  while( zend_hash_get_current_data_ex(data_hash, (void**) &data_entry, &data_pointer) == SUCCESS ) {
    // Get current key
    key_type = zend_hash_get_current_key_ex(data_hash, &key_str, &key_len, &key_nindex, false, &data_pointer);
    // Check key type
    if( key_type != HASH_KEY_IS_STRING ) {
      // Non-string key
      php_error(E_WARNING, "Partial array contains a non-string key");
    } else if( Z_TYPE_PP(data_entry) == IS_STRING ) {
      // String key, string value
      ckey.assign(key_str);
      tmpl.assign(Z_STRVAL_PP(data_entry));
      partials->insert(std::make_pair(ckey, node));
      mustache->tokenize(&tmpl, &(*partials)[ckey]);
    } else if( Z_TYPE_PP(data_entry) == IS_OBJECT ) {
      // String key, object value
      if( Z_OBJCE_PP(data_entry) == MustacheTemplate_ce_ptr ) {
        ckey.assign(key_str);
        mtPayload = (php_obj_MustacheTemplate *) zend_object_store_get_object(*data_entry TSRMLS_CC);
        partials->insert(std::make_pair(ckey, node));
        mustache->tokenize(mtPayload->tmpl, &(*partials)[ckey]);
      } else if( Z_OBJCE_PP(data_entry) == MustacheAST_ce_ptr ) {
        ckey.assign(key_str);
        maPayload = (php_obj_MustacheAST *) zend_object_store_get_object(*data_entry TSRMLS_CC);
        partials->insert(std::make_pair(ckey, node));
        
        // This is kind of hack-ish
        nodePtr = &(*partials)[ckey];
        nodePtr->type = mustache::Node::TypeContainer;
        nodePtr->child = maPayload->node;
      } else {
        php_error(E_WARNING, "Object not an instance of MustacheTemplate or MustacheAST");
      }
    } else {
      php_error(E_WARNING, "Partial array contains an invalid value");
    }
    zend_hash_move_forward_ex(data_hash, &data_pointer);
  }
}

bool mustache_parse_template_param(zval * tmpl, mustache::Mustache * mustache,
        mustache::Node ** node TSRMLS_DC)
{
  // Prepare template string
  if( Z_TYPE_P(tmpl) == IS_STRING ) {
    // Tokenize template
    char * tmpstr = Z_STRVAL_P(tmpl);
    *node = new mustache::Node();
    try {
      std::string templateStr(tmpstr/*, (size_t) Z_STRLEN_P(tmpl)*/);
      mustache->tokenize(&templateStr, *node);
    } catch(...) {
      delete *node; // Prevent leaks
      *node = NULL;
      throw;
    }
    return true;

  } else if( Z_TYPE_P(tmpl) == IS_OBJECT ) {
    // Use compiled template
    if( Z_OBJCE_P(tmpl) == MustacheTemplate_ce_ptr ) {
      php_obj_MustacheTemplate * mtPayload = 
              (php_obj_MustacheTemplate *) zend_object_store_get_object(tmpl TSRMLS_CC);
      
      if( mtPayload->tmpl == NULL ) {
        php_error(E_WARNING, "Empty MustacheTemplate");
        return false;
      } else {
        *node = new mustache::Node();
        try {
          mustache->tokenize(mtPayload->tmpl, *node);
        } catch(...) {
          delete *node; // Prevent leaks
          *node = NULL;
          throw;
        }
      }
      return true;
    } else if( Z_OBJCE_P(tmpl) == MustacheAST_ce_ptr ) {
      php_obj_MustacheAST * maPayload = 
              (php_obj_MustacheAST *) zend_object_store_get_object(tmpl TSRMLS_CC);
      if( maPayload->node == NULL ) {
        php_error(E_WARNING, "Empty MustacheAST");
        return false;
      }
      *node = maPayload->node;
      return true;
    } else {
      php_error(E_WARNING, "Object not an instance of MustacheTemplate or MustacheAST");
      return false;
    }
  } else {
    php_error(E_WARNING, "Invalid argument");
    return false;
  }
}



// Methods ---------------------------------------------------------------------

/* {{{ proto void __construct()
   */
PHP_METHOD(Mustache, __construct)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, Mustache_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} __construct */

/* {{{ proto boolean getEscapeByDefault()
   */
PHP_METHOD(Mustache, getEscapeByDefault)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, Mustache_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Main
    if( payload->mustache->getEscapeByDefault() ) {
      RETURN_TRUE;
    } else {
      RETURN_FALSE;
    }
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} getEscapeByDefault */

/* {{{ proto string getStartSequence()
   */
PHP_METHOD(Mustache, getStartSequence)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, Mustache_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Main
    const std::string & str = payload->mustache->getStartSequence();
    RETURN_STRING(str.c_str(), 1);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} getStartSequence */

/* {{{ proto string getStopSequence()
   */
PHP_METHOD(Mustache, getStopSequence)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, Mustache_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Main
    const std::string & str = payload->mustache->getStopSequence();
    RETURN_STRING(str.c_str(), 1);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} getStopSequence */

/* {{{ proto boolean setStartSequence(bool flag)
   */
PHP_METHOD(Mustache, setEscapeByDefault)
{
  try {
    // Custom parameters
    long flag = 0;
  
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Ol", 
            &_this_zval, Mustache_ce_ptr, &flag) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
  
    // Main
    payload->mustache->setEscapeByDefault((bool) flag != 0);
    RETURN_TRUE;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} setEscapeByDefault */

/* {{{ proto boolean setStartSequence(string str)
   */
PHP_METHOD(Mustache, setStartSequence)
{
  try {
    // Custom parameters
    char * str = NULL;
    long str_len = 0;
    
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Os", 
            &_this_zval, Mustache_ce_ptr, &str, &str_len) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Main
    payload->mustache->setStartSequence(str/*, str_len*/);
    RETURN_TRUE;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} setStartSequence */

/* {{{ proto boolean setStopSequence(string str)
   */
PHP_METHOD(Mustache, setStopSequence)
{
  try {
    // Custom parameters
    char * str = NULL;
    long str_len = 0;
    
    // Check parameters
    zval * _this_zval;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Os", 
            &_this_zval, Mustache_ce_ptr, &str, &str_len) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Main
    payload->mustache->setStopSequence(str/*, str_len*/);
    RETURN_TRUE;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} setStartSequence */


/* {{{ proto MustacheCode compile(string template)
   */
PHP_METHOD(Mustache, compile)
{
  try {
    // Custom parameters
    zval * tmpl = NULL;
    zval * partials = NULL;
  
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Oz|z", 
            &_this_zval, Mustache_ce_ptr, &tmpl, &partials) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Prepare template tree
    mustache::Node templateNode;
    mustache::Node * templateNodePtr = &templateNode;
    if( !mustache_parse_template_param(tmpl, payload->mustache, &templateNodePtr TSRMLS_CC) ) {
      RETURN_FALSE;
      return;
    }
    
    // Prepare partials
    mustache::Node::Partials templatePartials;
    mustache_parse_partials_param(partials, payload->mustache, &templatePartials TSRMLS_CC);
    
    // Compile
    uint8_t * codes;
    size_t codes_length;
    payload->mustache->compile(templateNodePtr, &templatePartials, &codes, &codes_length);
    
    // Initialize new object
    object_init_ex(return_value, MustacheCode_ce_ptr);
    php_obj_MustacheCode * intern = 
            (php_obj_MustacheCode *) zend_objects_get_address(return_value TSRMLS_CC);
    intern->codes = codes;
    intern->length = codes_length;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} compile */

/* {{{ proto string execute(MustacheCode code)
   */
PHP_METHOD(Mustache, execute)
{
  try {
    // Custom parameters
    zval * code = NULL;
    zval * data = NULL;
    
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "OOz", 
            &_this_zval, Mustache_ce_ptr, &code, MustacheCode_ce_ptr, &data) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Prepare code
    php_obj_MustacheCode * codePayload = 
            (php_obj_MustacheCode *) zend_object_store_get_object(code TSRMLS_CC);
    
    // Prepare template data
    mustache::Data templateData;
    mustache::Data * templateDataPtr = &templateData;
    if( !mustache_parse_data_param(data, payload->mustache, &templateDataPtr TSRMLS_CC) ) {
      RETURN_FALSE;
      return;
    }
    
    // Execute bytecode
    std::string output;
    payload->mustache->execute(codePayload->codes, codePayload->length, templateDataPtr, &output);
    
    // Output
    RETURN_STRING(output.c_str(), 1); // Yes reallocate
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} execute */

/* {{{ proto MustacheAST parse(string template)
   */
PHP_METHOD(Mustache, parse)
{
  try {
    // Custom parameters
    zval * tmpl = NULL;
  
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Oz", 
            &_this_zval, Mustache_ce_ptr, &tmpl) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Check template parameter
    mustache::Node * templateNodePtr = NULL;
    if( !mustache_parse_template_param(tmpl, payload->mustache, &templateNodePtr TSRMLS_CC) ) {
      delete templateNodePtr;
      RETURN_FALSE;
      return;
    }
    
    // Handle return value
    if( Z_TYPE_P(tmpl) == IS_STRING ) {
      if( MustacheAST_ce_ptr == NULL ) {
        delete templateNodePtr;
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class MustacheAST does not exist");
        RETURN_FALSE;
        return;
      }
      
      // Initialize new object
      object_init_ex(return_value, MustacheAST_ce_ptr);
      php_obj_MustacheAST * intern = 
              (php_obj_MustacheAST *) zend_objects_get_address(return_value TSRMLS_CC);
      
      intern->node = templateNodePtr;
      
    // Ref - not sure if this is required
//    Z_SET_REFCOUNT_P(return_value, 1);
//    Z_SET_ISREF_P(return_value);
    
    } else if( Z_TYPE_P(tmpl) == IS_OBJECT ) {
      // Handle return value for object parameter
      // @todo return the object itself?
      RETURN_TRUE;
    }
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} parse */

/* {{{ proto string render(mixed template, array data, array partials)
   */
PHP_METHOD(Mustache, render)
{
  try {
    // Custom parameters
    zval * tmpl = NULL;
    zval * data = NULL;
    zval * partials = NULL;
    
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Ozz|a/", 
            &_this_zval, Mustache_ce_ptr, &tmpl, &data, &partials) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Prepare template tree
    mustache::Node templateNode;
    mustache::Node * templateNodePtr = &templateNode;
    if( !mustache_parse_template_param(tmpl, payload->mustache, &templateNodePtr TSRMLS_CC) ) {
      RETURN_FALSE;
      return;
    }
    
    // Prepare template data
    mustache::Data templateData;
    mustache::Data * templateDataPtr = &templateData;
    if( !mustache_parse_data_param(data, payload->mustache, &templateDataPtr TSRMLS_CC) ) {
      RETURN_FALSE;
      return;
    }
    
    // Tokenize partials
    mustache::Node::Partials templatePartials;
    mustache_parse_partials_param(partials, payload->mustache, &templatePartials TSRMLS_CC);
    
    // Reserve length of template
    std::string output;
    if( Z_TYPE_P(tmpl) == IS_STRING ) {
      output.reserve(Z_STRLEN_P(tmpl));
    }
    
    // Render template
    payload->mustache->render(templateNodePtr, templateDataPtr, &templatePartials, &output);
    
    // Output
    RETURN_STRING(output.c_str(), 1); // Yes reallocate
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} render */

/* {{{ proto array tokenize(string template)
   */
PHP_METHOD(Mustache, tokenize)
{
  try {
    // Custom parameters
    char * template_str = NULL;
    long template_len = 0;
  
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Os", 
            &_this_zval, Mustache_ce_ptr, &template_str, &template_len) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Assign template to string
    std::string templateStr(template_str/*, template_len*/);
    
    // Tokenize template
    mustache::Node root;
    payload->mustache->tokenize(&templateStr, &root);
    
    // Convert to PHP array
    mustache_node_to_zval(&root, return_value TSRMLS_CC);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} tokenize */

/* {{{ proto array debugDataStructure(array data)
   */
PHP_METHOD(Mustache, debugDataStructure)
{
  try {
    // Custom parameters
    zval * data = NULL;
    
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "Oz", 
            &_this_zval, Mustache_ce_ptr, &data) == FAILURE) {
      throw PhpInvalidParameterException();
    }

    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_Mustache * payload = 
            (php_obj_Mustache *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Prepare template data
    mustache::Data templateData;
    mustache_data_from_zval(&templateData, data TSRMLS_CC);

    // Reverse template data
    mustache_data_to_zval(&templateData, return_value TSRMLS_CC);
  
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} debugDataStructure */



// Argument Info ---------------------------------------------------------------

ZEND_BEGIN_ARG_INFO_EX(Mustache____construct_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__getEscapeByDefault_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__getStartSequence_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__getStopSequence_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__setEscapeByDefault_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, escapeByDefault)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__setStartSequence_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, startSequence)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__setStopSequence_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, stopSequence)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__compile_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, tmpl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__execute_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, code)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__parse_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, tmpl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__render_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 3)
	ZEND_ARG_INFO(0, str)
        ZEND_ARG_INFO(0, vars)
        ZEND_ARG_INFO(0, partials)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__tokenize_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, tmpl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mustache__debugDataStructure_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
        ZEND_ARG_INFO(0, vars)
ZEND_END_ARG_INFO()



// Method Entries --------------------------------------------------------------

static zend_function_entry Mustache_methods[] = {
  PHP_ME(Mustache, __construct, Mustache____construct_args, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
  PHP_ME(Mustache, getEscapeByDefault, Mustache__getEscapeByDefault_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, getStartSequence, Mustache__getStartSequence_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, getStopSequence, Mustache__getStopSequence_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, setEscapeByDefault, Mustache__setEscapeByDefault_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, setStartSequence, Mustache__setStartSequence_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, setStopSequence, Mustache__setStopSequence_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, compile, Mustache__compile_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, execute, Mustache__execute_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, parse, Mustache__parse_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, render, Mustache__render_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, tokenize, Mustache__tokenize_args, ZEND_ACC_PUBLIC)
  PHP_ME(Mustache, debugDataStructure, Mustache__debugDataStructure_args, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};



// MINIT -----------------------------------------------------------------------

PHP_MINIT_FUNCTION(mustache_mustache)
{
  try {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "Mustache", Mustache_methods);
    ce.create_object = Mustache_obj_create;
    Mustache_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    memcpy(&Mustache_obj_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    Mustache_obj_handlers.clone_obj = NULL;

    return SUCCESS;
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
    return FAILURE;
  }
}
