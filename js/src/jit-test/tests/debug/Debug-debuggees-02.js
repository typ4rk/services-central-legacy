// |jit-test| debug
// The array returned by getDebuggees is just a snapshot, not live
var dbg = new Debug;
var a1 = dbg.getDebuggees();
var g = newGlobal('new-compartment');
var gw = dbg.addDebuggee(g);
assertEq(gw instanceof Debug.Object, true);
var a2 = dbg.getDebuggees();
assertEq(a2.length, 1);
assertEq(a2[0], gw);
assertEq(a1.length, 0);