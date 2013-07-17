--TEST--
MustacheAST::__sleep() member function
--SKIPIF--
<?php 
if( !extension_loaded('mustache') ) die('skip ');
 ?>
--FILE--
<?php
$tmpl = new MustacheAST("MU         MU         test ");
$serial = serialize($tmpl);
var_dump($serial);
$orig = unserialize($serial);
var_dump($orig);
?>
--EXPECT--
string(84) "O:11:"MustacheAST":1:{s:12:"binaryString";s:33:"MU         MU         test ";}"
object(MustacheAST)#2 (1) {
  ["binaryString"]=>
  string(33) "MU         MU         test "
}