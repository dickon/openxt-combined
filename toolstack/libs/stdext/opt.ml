(*
 * Copyright (C) 2006-2007 XenSource Ltd.
 * Copyright (C) 2008      Citrix Ltd.
 * Author Vincent Hanquez <vincent.hanquez@eu.citrix.com>
 * Author Dave Scott <dave.scott@eu.citrix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *)
let iter f = function
	| Some x -> f x
	| None -> ()

let map f = function
	| Some x -> Some(f x)
	| None -> None

let default d = function
	| Some x -> x
	| None -> d

let unbox = function
	| Some x -> x
	| None -> raise Not_found

let is_boxed = function
	| Some _ -> true
	| None -> false

let to_list = function
	| Some x -> [x]
	| None -> []

let fold_left f accu = function
	| Some x -> f accu x
	| None -> accu

let fold_right f opt accu =
	match opt with
	| Some x -> f x accu
	| None -> accu
