------------------ MODULE beam_scheduler ------------------
EXTENDS Integers, Sequences, FiniteSets

CONSTANTS ProcessId, MaxReductions

VARIABLES processState, reductions, activeProcess

Vars == <<processState, reductions, activeProcess>>

States == {"RUNNABLE", "RUNNING", "WAITING", "EXITED"}

Init ==
    /\ processState = [p IN ProcessId |-> "RUNNABLE"]
    /\ reductions = [p IN ProcessId |-> MaxReductions]
    /\ activeProcess \in ProcessId

Step(p) ==
    /\ activeProcess = p
    /\ processState[p] = "RUNNING"
    /\ reductions[p] > 0
    /\ reductions' = [reductions EXCEPT ![p] = reductions[p] - 1]
    /\ UNCHANGED <<processState, activeProcess>>

Preempt(p) ==
    /\ activeProcess = p
    /\ processState[p] = "RUNNING"
    /\ reductions[p] = 0
    /\ processState' = [processState EXCEPT ![p] = "RUNNABLE"]
    /\ reductions' = [reductions EXCEPT ![p] = MaxReductions]
    /\ \E nextP \in ProcessId : activeProcess' = nextP

Next ==
    \E p \in ProcessId : Step(p) \/ Preempt(p)

Spec == Init /\ [][Next]_Vars

NoDeadlock == \E p \in ProcessId : processState[p] /= "EXITED"
===========================================================
