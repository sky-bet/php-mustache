--TEST--
MustacheAST::__toString() member function
--SKIPIF--
<?php 
if( !extension_loaded('mustache') ) die('skip ');
 ?>
--FILE--
<?php
$tmpl = new MustacheAST("MU         MU         test ");
var_dump($tmpl->__toString());
var_dump((string) $tmpl);
?>
--EXPECT--
string(33) "MU         MU         test "
string(33) "MU         MU         test "