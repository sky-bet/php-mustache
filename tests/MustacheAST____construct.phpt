--TEST--
MustacheAST::__construct() member function
--SKIPIF--
<?php 
if( !extension_loaded('mustache') ) die('skip ');
 ?>
--FILE--
<?php
$tmpl = new MustacheAST("MU         MU         test ");
var_dump(get_class($tmpl));
var_dump($tmpl->__toString());
?>
--EXPECT--
string(11) "MustacheAST"
string(33) "MU         MU         test "