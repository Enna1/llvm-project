; Ensure that 1 is added to the high 20 bits if bit 11 of the low part is 1
define i32 @lw_sw_constant(i32 %a) nounwind {
; TODO: the addi should be folded in to the lw/sw
; RV32I-LABEL: lw_sw_constant:
; RV32I:       # %bb.0:
; RV32I-NEXT:    lui a1, 912092
; RV32I-NEXT:    addi a2, a1, -273
; RV32I-NEXT:    lw a1, 0(a2)
; RV32I-NEXT:    sw a0, 0(a2)
; RV32I-NEXT:    addi a0, a1, 0
; RV32I-NEXT:    jalr zero, ra, 0
  %1 = inttoptr i32 3735928559 to i32*
  %2 = load volatile i32, i32* %1
  store i32 %a, i32* %1
  ret i32 %2
}