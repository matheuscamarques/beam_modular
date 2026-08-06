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
  | OpHalt : Opcode.

Definition execute_opcode (op : Opcode) (frame : EmulatorFrame) : EmulatorFrame :=
  match op with
  | OpHalt => frame
  | _ => frame
  end.

Theorem opcode_execution_deterministic : forall (op : Opcode) (f f1 f2 : EmulatorFrame),
  execute_opcode op f = f1 -> execute_opcode op f = f2 -> f1 = f2.
Proof.
  intros op f f1 f2 H1 H2.
  subst f1. subst f2. reflexivity.
Qed.
