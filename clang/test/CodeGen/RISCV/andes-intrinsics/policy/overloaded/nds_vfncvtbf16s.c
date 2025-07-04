// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py UTC_ARGS: --version 2
// REQUIRES: riscv-registered-target
// RUN: %clang_cc1 -triple riscv64 -target-feature +v \
// RUN:   -target-feature +xandesvbfhcvt -disable-O0-optnone \
// RUN:   -emit-llvm %s -o - | opt -S -passes=mem2reg | \
// RUN:   FileCheck --check-prefix=CHECK-RV64 %s

#include <andes_vector.h>

// CHECK-RV64-LABEL: define dso_local <vscale x 1 x bfloat> @test_nds_vfncvt_bf16_s_bf16mf4_tu
// CHECK-RV64-SAME: (<vscale x 1 x bfloat> [[VD:%.*]], <vscale x 1 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0:[0-9]+]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 1 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv1bf16.nxv1f32.i64(<vscale x 1 x bfloat> [[VD]], <vscale x 1 x float> [[VS2]], i64 7, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 1 x bfloat> [[TMP0]]
//
vbfloat16mf4_t test_nds_vfncvt_bf16_s_bf16mf4_tu(vbfloat16mf4_t vd,
                                                 vfloat32mf2_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 2 x bfloat> @test_nds_vfncvt_bf16_s_bf16mf2_tu
// CHECK-RV64-SAME: (<vscale x 2 x bfloat> [[VD:%.*]], <vscale x 2 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 2 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv2bf16.nxv2f32.i64(<vscale x 2 x bfloat> [[VD]], <vscale x 2 x float> [[VS2]], i64 7, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 2 x bfloat> [[TMP0]]
//
vbfloat16mf2_t test_nds_vfncvt_bf16_s_bf16mf2_tu(vbfloat16mf2_t vd,
                                                 vfloat32m1_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 4 x bfloat> @test_nds_vfncvt_bf16_s_bf16m1_tu
// CHECK-RV64-SAME: (<vscale x 4 x bfloat> [[VD:%.*]], <vscale x 4 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 4 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv4bf16.nxv4f32.i64(<vscale x 4 x bfloat> [[VD]], <vscale x 4 x float> [[VS2]], i64 7, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 4 x bfloat> [[TMP0]]
//
vbfloat16m1_t test_nds_vfncvt_bf16_s_bf16m1_tu(vbfloat16m1_t vd,
                                               vfloat32m2_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 8 x bfloat> @test_nds_vfncvt_bf16_s_bf16m2_tu
// CHECK-RV64-SAME: (<vscale x 8 x bfloat> [[VD:%.*]], <vscale x 8 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 8 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv8bf16.nxv8f32.i64(<vscale x 8 x bfloat> [[VD]], <vscale x 8 x float> [[VS2]], i64 7, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 8 x bfloat> [[TMP0]]
//
vbfloat16m2_t test_nds_vfncvt_bf16_s_bf16m2_tu(vbfloat16m2_t vd,
                                               vfloat32m4_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 16 x bfloat> @test_nds_vfncvt_bf16_s_bf16m4_tu
// CHECK-RV64-SAME: (<vscale x 16 x bfloat> [[VD:%.*]], <vscale x 16 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 16 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv16bf16.nxv16f32.i64(<vscale x 16 x bfloat> [[VD]], <vscale x 16 x float> [[VS2]], i64 7, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 16 x bfloat> [[TMP0]]
//
vbfloat16m4_t test_nds_vfncvt_bf16_s_bf16m4_tu(vbfloat16m4_t vd,
                                               vfloat32m8_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 1 x bfloat> @test_nds_vfncvt_bf16_s_bf16mf4_rm_tu
// CHECK-RV64-SAME: (<vscale x 1 x bfloat> [[VD:%.*]], <vscale x 1 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 1 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv1bf16.nxv1f32.i64(<vscale x 1 x bfloat> [[VD]], <vscale x 1 x float> [[VS2]], i64 0, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 1 x bfloat> [[TMP0]]
//
vbfloat16mf4_t test_nds_vfncvt_bf16_s_bf16mf4_rm_tu(vbfloat16mf4_t vd,
                                                    vfloat32mf2_t vs2,
                                                    size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, __RISCV_FRM_RNE, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 2 x bfloat> @test_nds_vfncvt_bf16_s_bf16mf2_rm_tu
// CHECK-RV64-SAME: (<vscale x 2 x bfloat> [[VD:%.*]], <vscale x 2 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 2 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv2bf16.nxv2f32.i64(<vscale x 2 x bfloat> [[VD]], <vscale x 2 x float> [[VS2]], i64 0, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 2 x bfloat> [[TMP0]]
//
vbfloat16mf2_t test_nds_vfncvt_bf16_s_bf16mf2_rm_tu(vbfloat16mf2_t vd,
                                                    vfloat32m1_t vs2,
                                                    size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, __RISCV_FRM_RNE, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 4 x bfloat> @test_nds_vfncvt_bf16_s_bf16m1_rm_tu
// CHECK-RV64-SAME: (<vscale x 4 x bfloat> [[VD:%.*]], <vscale x 4 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 4 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv4bf16.nxv4f32.i64(<vscale x 4 x bfloat> [[VD]], <vscale x 4 x float> [[VS2]], i64 0, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 4 x bfloat> [[TMP0]]
//
vbfloat16m1_t test_nds_vfncvt_bf16_s_bf16m1_rm_tu(vbfloat16m1_t vd,
                                                  vfloat32m2_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, __RISCV_FRM_RNE, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 8 x bfloat> @test_nds_vfncvt_bf16_s_bf16m2_rm_tu
// CHECK-RV64-SAME: (<vscale x 8 x bfloat> [[VD:%.*]], <vscale x 8 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 8 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv8bf16.nxv8f32.i64(<vscale x 8 x bfloat> [[VD]], <vscale x 8 x float> [[VS2]], i64 0, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 8 x bfloat> [[TMP0]]
//
vbfloat16m2_t test_nds_vfncvt_bf16_s_bf16m2_rm_tu(vbfloat16m2_t vd,
                                                  vfloat32m4_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, __RISCV_FRM_RNE, vl);
}

// CHECK-RV64-LABEL: define dso_local <vscale x 16 x bfloat> @test_nds_vfncvt_bf16_s_bf16m4_rm_tu
// CHECK-RV64-SAME: (<vscale x 16 x bfloat> [[VD:%.*]], <vscale x 16 x float> [[VS2:%.*]], i64 noundef [[VL:%.*]]) #[[ATTR0]] {
// CHECK-RV64-NEXT:  entry:
// CHECK-RV64-NEXT:    [[TMP0:%.*]] = call <vscale x 16 x bfloat> @llvm.riscv.nds.vfncvt.bf16.s.nxv16bf16.nxv16f32.i64(<vscale x 16 x bfloat> [[VD]], <vscale x 16 x float> [[VS2]], i64 0, i64 [[VL]])
// CHECK-RV64-NEXT:    ret <vscale x 16 x bfloat> [[TMP0]]
//
vbfloat16m4_t test_nds_vfncvt_bf16_s_bf16m4_rm_tu(vbfloat16m4_t vd,
                                                  vfloat32m8_t vs2, size_t vl) {
  return __riscv_nds_vfncvt_bf16_tu(vd, vs2, __RISCV_FRM_RNE, vl);
}
