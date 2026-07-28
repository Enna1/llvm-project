; RUN: opt -passes='slp-vectorizer,loop-unroll' -S %s | FileCheck %s
; REQUIRES: x86-registered-target

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@data = constant [14 x i8] c"\06\02\02\0D\FF\F0\13\0D\FA\12\FB\FB\F2\0A"

define i64 @test() #0 {
; CHECK-LABEL: define i64 @test(
entry:
  br label %outer

outer:
; CHECK:       outer:
; CHECK-NEXT:    %outer.iv = phi i64
  %outer.iv = phi i64 [ %outer.iv.next, %outer.latch ], [ 0, %entry ]
  br label %inner

inner:
  %inner.iv = phi i64 [ %inner.iv.next, %inner ], [ 0, %outer ]
  %sum = phi i64 [ %sum.next, %inner ], [ 153, %outer ]
  %unused.add = add i32 0, 0
  %unused.sext = sext i32 0 to i64
  %unused.add64 = add i64 0, 0
  %unused.cmp = icmp eq i64 0, 0
  %ptr = getelementptr i8, ptr @data, i64 %inner.iv
  %value = load i8, ptr %ptr, align 1
  %value.ext = sext i8 %value to i32
  %value.mul = mul i32 %value.ext, 3
  %value.mul.ext = sext i32 %value.mul to i64
  %sum.next = add i64 %sum, %value.mul.ext
  %unused.select = select i1 false, i64 0, i64 0
  %inner.iv.next = add i64 %inner.iv, 1
  %inner.exit = icmp eq i64 %inner.iv.next, 14
  br i1 %inner.exit, label %inner.exit.block, label %inner

inner.exit.block:
  %outer.iv.trunc = trunc i64 %outer.iv to i32
  %outer.offset = add i32 %outer.iv.trunc, -283
  %sum.trunc = trunc i64 %sum.next to i32
  %exit.value = add i32 %outer.offset, %sum.trunc
  %outer.exit = icmp slt i32 %exit.value, 0
  br i1 %outer.exit, label %outer.latch, label %exit

outer.latch:
; CHECK:       outer.latch:
; CHECK-NEXT:    %outer.iv.next = add i64 %outer.iv, 1
; CHECK-NEXT:    br label %outer
  %outer.iv.next = add i64 %outer.iv, 1
  br label %outer

exit:
  ret i64 0
}

attributes #0 = { "target-cpu"="x86-64" }
