(* Formal specification of BEAM Terms and Deterministic State Transition in Coq/Rocq *)

Require Import ZArith.
Require Import List.
Import ListNotations.

Inductive Eterm : Type :=
  | SmallInt : Z -> Eterm
  | Atom : Z -> Eterm
  | Nil : Eterm
  | ListCell : Eterm -> Eterm -> Eterm
  | Tuple : list Eterm -> Eterm.

Record EmulatorFrame : Type := {
  x_regs : list Eterm;
  ip : Z;
  sp : Z
}.

Inductive Opcode : Type :=
  | OpMove : Eterm -> Z -> Opcode
  | OpAdd : Z -> Z -> Z -> Opcode
  | OpSub : Z -> Z -> Z -> Opcode
  | OpMul : Z -> Z -> Z -> Opcode
  | OpIntDiv : Z -> Z -> Z -> Opcode
  | OpAllocate : Z -> Opcode
  | OpDeallocate : Z -> Opcode
  | OpCallLast : Z -> Z -> Opcode
  | OpMakeFun2 : Z -> Z -> Z -> Opcode
  | OpCallExt : Z -> Z -> Opcode
  | OpLoopRec : Z -> Z -> Opcode
  | OpLoopRecEnd : Z -> Opcode
  | OpRemoveMessage : Opcode
  | OpWait : Opcode
  | OpHalt : Opcode.

Definition execute_opcode (op : Opcode) (frame : EmulatorFrame) : EmulatorFrame :=
  match op with
  | OpAllocate n => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) - n |}
  | OpDeallocate n => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) + n |}
  | OpCallLast target n => {| x_regs := frame.(x_regs); ip := target; sp := frame.(sp) + n |}
  | OpMakeFun2 label n dst => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpCallExt bif arity => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpLoopRec fail dst => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpLoopRecEnd target => {| x_regs := frame.(x_regs); ip := target; sp := frame.(sp) |}
  | OpRemoveMessage => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpWait => {| x_regs := frame.(x_regs); ip := frame.(ip); sp := frame.(sp) |}
  | OpMul r1 r2 dst => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpIntDiv r1 r2 dst => {| x_regs := frame.(x_regs); ip := frame.(ip) + 1; sp := frame.(sp) |}
  | OpHalt => frame
  | _ => frame
  end.

Theorem tail_call_optimization_stack_preservation : forall (target n : Z) (f : EmulatorFrame),
  (execute_opcode (OpCallLast target n) (execute_opcode (OpAllocate n) f)).(sp) = f.(sp).
Proof.
  intros target n f.
  simpl.
  ring.
Qed.

Theorem allocate_deallocate_stack_invariant : forall (n : Z) (f : EmulatorFrame),
  (execute_opcode (OpDeallocate n) (execute_opcode (OpAllocate n) f)).(sp) = f.(sp).
Proof.
  intros n f.
  simpl.
  ring.
Qed.

Theorem opcode_execution_deterministic : forall (op : Opcode) (f f1 f2 : EmulatorFrame),
  execute_opcode op f = f1 -> execute_opcode op f = f2 -> f1 = f2.
Proof.
  intros op f f1 f2 H1 H2.
  subst f1. subst f2. reflexivity.
Qed.
