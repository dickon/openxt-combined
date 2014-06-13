(* $Id: length.mli,v 1.1 2007-11-06 21:06:18 weis Exp $

A testbed file for private type abbreviation definitions.

We define a Length module to implement positive integers.

*)

type t = private int;;

val make : int -> t;;

external from : t -> int = "%identity";;
