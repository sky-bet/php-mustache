
#include "php_mustache.hpp"



// Declarations ----------------------------------------------------------------

PHP_METHOD(MustacheTemplate, __construct);
PHP_METHOD(MustacheTemplate, __sleep);
PHP_METHOD(MustacheTemplate, __toString);
PHP_METHOD(MustacheTemplate, __wakeup);



// Argument Info ---------------------------------------------------------------

ZEND_BEGIN_ARG_INFO_EX(MustacheTemplate____construct_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
    ZEND_ARG_INFO(0, vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(MustacheTemplate____sleep_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(MustacheTemplate____toString_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(MustacheTemplate____wakeup_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()



// Class Entries ---------------------------------------------------------------

zend_class_entry * MustacheTemplate_ce_ptr;



// Method Entries --------------------------------------------------------------

static zend_function_entry MustacheTemplate_methods[] = {
  PHP_ME(MustacheTemplate, __construct, MustacheTemplate____construct_args, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
  PHP_ME(MustacheTemplate, __sleep, MustacheTemplate____sleep_args, ZEND_ACC_PUBLIC)
  PHP_ME(MustacheTemplate, __toString, MustacheTemplate____toString_args, ZEND_ACC_PUBLIC)
  PHP_ME(MustacheTemplate, __wakeup, MustacheTemplate____wakeup_args, ZEND_ACC_PUBLIC)
  { NULL, NULL, NULL }
};



// Object Handlers -------------------------------------------------------------

static zend_object_handlers MustacheTemplate_obj_handlers;

static void MustacheTemplate_obj_free(void *object TSRMLS_DC)
{
  try {
    php_obj_MustacheTemplate * payload = (php_obj_MustacheTemplate *) object;
    
    if( payload->tmpl != NULL ) {
      delete payload->tmpl;
    }
    
    zend_object_std_dtor((zend_object *)object TSRMLS_CC);

    efree(object);
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}

static zend_object_value MustacheTemplate_obj_create(zend_class_entry *class_type TSRMLS_DC)
{
  zend_object_value retval;
  
  try {
    php_obj_MustacheTemplate * payload;
    zval * tmp;

    payload = (php_obj_MustacheTemplate *) emalloc(sizeof(php_obj_MustacheTemplate));
    memset(payload, 0, sizeof(php_obj_MustacheTemplate));
    
    zend_object_std_init((zend_object *) payload, class_type TSRMLS_CC);
    
#if PHP_VERSION_ID < 50399
    zend_hash_copy(payload->obj.properties, &(class_type->default_properties),
        (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval*));
#else
    object_properties_init(&payload->obj, class_type);
#endif
    
    payload->tmpl = NULL; //new std::string();

    retval.handle = zend_objects_store_put(payload, NULL, (zend_objects_free_object_storage_t) MustacheTemplate_obj_free, NULL TSRMLS_CC);
    retval.handlers = &MustacheTemplate_obj_handlers;
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
  
  return retval;
}



// MINIT -----------------------------------------------------------------------

PHP_MINIT_FUNCTION(mustache_template)
{
  try {
    zend_class_entry ce;

    memcpy(&MustacheTemplate_obj_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    MustacheTemplate_obj_handlers.clone_obj = NULL;
    
    INIT_CLASS_ENTRY(ce, "MustacheTemplate", MustacheTemplate_methods);
    ce.create_object = MustacheTemplate_obj_create;
    
    MustacheTemplate_ce_ptr = zend_register_internal_class(&ce TSRMLS_CC);
    MustacheTemplate_ce_ptr->create_object = MustacheTemplate_obj_create;
    
    zend_declare_property_null(MustacheTemplate_ce_ptr, ZEND_STRL("template"), ZEND_ACC_PROTECTED TSRMLS_CC);
    
    return SUCCESS;
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
    return FAILURE;
  }
}



// Methods ---------------------------------------------------------------------

/* {{{ proto void __construct(string tmpl)
   */
PHP_METHOD(MustacheTemplate, __construct)
{
  try {
    // Custom parameters
    char * template_str = NULL;
    long template_len = 0;
    
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O|s", 
            &_this_zval, MustacheTemplate_ce_ptr, &template_str, &template_len) == FAILURE) {
      throw PhpInvalidParameterException();
    }
    
    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_MustacheTemplate * payload = 
            (php_obj_MustacheTemplate *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Check if data was null
    if( template_len > 0 && template_str != NULL ) {
      // Store template
      payload->tmpl = new std::string(template_str/*, template_len*/);
    }
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} __construct */

/* {{{ proto void __sleep()
   */
PHP_METHOD(MustacheTemplate, __sleep)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, MustacheTemplate_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }
    
    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_MustacheTemplate * payload = 
            (php_obj_MustacheTemplate *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    array_init(return_value);
    
    if( payload->tmpl != NULL ) {
      // Store
      zend_update_property_stringl(MustacheTemplate_ce_ptr, _this_zval, 
              ZEND_STRL("template"), 
            (char *) payload->tmpl->c_str(), 
            payload->tmpl->length() TSRMLS_CC);
        
      add_next_index_string(return_value, "template", 1);
    }
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} __sleep */

/* {{{ proto string __toString()
   */
PHP_METHOD(MustacheTemplate, __toString)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, MustacheTemplate_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }
    
    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_MustacheTemplate * payload = 
            (php_obj_MustacheTemplate *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Check payload
    if( payload->tmpl == NULL ) {
      throw InvalidParameterException("MustacheTemplate was not initialized properly");
    }
    
    // Return
    RETURN_STRING(payload->tmpl->c_str(), 1); // Yes reallocate
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} __toString */


/* {{{ proto void __wakeup()
   */
PHP_METHOD(MustacheTemplate, __wakeup)
{
  try {
    // Check parameters
    zval * _this_zval = NULL;
    if( zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), (char *) "O", 
            &_this_zval, MustacheTemplate_ce_ptr) == FAILURE) {
      throw PhpInvalidParameterException();
    }
    
    // Class parameters
    _this_zval = getThis();
    zend_class_entry * _this_ce = Z_OBJCE_P(_this_zval);
    php_obj_MustacheTemplate * payload = 
            (php_obj_MustacheTemplate *) zend_object_store_get_object(_this_zval TSRMLS_CC);
    
    // Get object properties
    // @todo should be able to convert this to use zend_hash_find
    int key_type = 0;
    char * key_str = NULL;
    uint key_len = 0;
    ulong key_nindex = 0;
    HashTable * data_hash = NULL;
    HashPosition data_pointer = NULL;
    zval **data_entry = NULL;
    long data_count = 0;
    if( Z_OBJ_HT_P(_this_zval)->get_properties != NULL ) {
      data_hash = Z_OBJ_HT_P(_this_zval)->get_properties(_this_zval TSRMLS_CC);
      data_count = zend_hash_num_elements(data_hash);
    }
    if( data_hash != NULL ) {
      char *prop_name, *class_name;
      zend_hash_internal_pointer_reset_ex(data_hash, &data_pointer);
      while( zend_hash_get_current_data_ex(data_hash, (void**) &data_entry, &data_pointer) == SUCCESS ) {
        if( zend_hash_get_current_key_ex(data_hash, &key_str, &key_len, 
                &key_nindex, false, &data_pointer) == HASH_KEY_IS_STRING ) {
#if PHP_API_VERSION >= 20100412
          zend_unmangle_property_name(key_str, key_len-1, (const char **) &class_name, (const char **) &prop_name);
#else
          zend_unmangle_property_name(key_str, key_len-1, &class_name, &prop_name);
#endif
          if( strcmp(prop_name, "template") == 0 && Z_TYPE_PP(data_entry) == IS_STRING ) {
            if( payload->tmpl == NULL ) {
              payload->tmpl = new std::string(Z_STRVAL_PP(data_entry));
            } else {
              payload->tmpl->assign(Z_STRVAL_PP(data_entry));
            }
          }
        }
        zend_hash_move_forward_ex(data_hash, &data_pointer);
      }
    }
    
  } catch(...) {
    mustache_exception_handler(TSRMLS_C);
  }
}
/* }}} __wakeup */
