#!/usr/bin/env thrift

namespace py slidinghyper
namespace js slidingHyper

exception InternalError {
  1: string message;
}

service SlidingHyperService {
  string ping(),

  void add_many(
    1:i32 timestamp,
    2:string key,
    3:list<string> values
  ) throws (1:InternalError error),

  i32 card(
    1:i32 timestamp,
    2:i32 window,
    3:list<string> keys,
    4:list<string> values
  ) throws (1:InternalError error),

  void flush(),

  # All deprecated mess.
  void add(
    1:i32 timestamp,
    2:string key,
    3:string value
  ),

  i32 get(
    1:i32 timestamp,
    2:i16 window,
    3:string key
  ),

  i32 get_union(
    1:i32 timestamp,
    2:i16 window,
    3:list<string> keys
  ),

  i32 get_with_element(
    1:i32 timestamp,
    2:i16 window,
    3:string key,
    4:string value
  ),

  i32 get_union_with_element(
    1:i32 timestamp,
    2:i16 window,
    3:list<string> keys,
    4:string value
  ),
}
