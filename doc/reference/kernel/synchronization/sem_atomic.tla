(*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *)

---- MODULE sem_atomic ----
EXTENDS TLC, Integers, Sequences, FiniteSets

CONSTANTS A, B, C, D, E, F,
SEM_NOT_INITIALIZED, SEM_INITIALIZING, SEM_INITIALIZED

(* Tradeoff between thoroughness and speed.
 * More processes allows more complex behavior, but also
 * take longer.
 *)
Proc == {A, B, C, D, E}

(* This is a rather simplified version of the atomic semaphore implementation,
 * modelled in PlusCal (which then gets compiled down to TLA+ to actually be run.)
 *
 *)
perms == Permutations(Proc)

(* Larger values are more thorough but much slower. *)
MAX_SEM_LIMIT == Cardinality(Proc)

(*--algorithm sem_atomic
{
  (* A fair amount of complexity for what boils down to
   * something conceptually simple. Essentially:
   * 1. either:
   *     a. issue a sem_init call with a count > 0
   *     b. issue a sem_init call, then a sem_give
   * 2. parallel_while_1:
   *     either:
   *         a. issue a sem_take, then, if successful, a sem_give.
   *         b. issue a sem_reset call followed by a sem_give.
   *         c. issue a sem_give call.
   *
   * There is a delicate balance with labels. We want more
   * labels as it gives us better indications of coverage, but
   * more labels mean more possible states, which very quickly gets
   * very slow.
   *)
  variables
    sem_init_state = SEM_NOT_INITIALIZED,
    sem_limit,
    max_in_critical_section,
    act_in_critical_section = 0,
    (* these are the only "actual" state variables. *)
    sem_count,
    sem_sleepers

  (* `fair` means that all processes eventually get to run.
   * See discussion on the `main_loop_done` label below.
   *)
  fair process (P \in Proc)
  {
    maybe_init:
      (* Do sem_init exactly once - note that as
       * sem_init isn't really threadsafe I'm
       * just treating it as one atomic step here.
       *)
      either {
          await sem_init_state = SEM_NOT_INITIALIZED;
          sem_init_state := SEM_INITIALIZING;
        sem_init:
          sem_sleepers := {};
          (* Choose arbitrary valid starting count and limit. *)
          with (chosen_limit \in 1..MAX_SEM_LIMIT) {
              sem_limit := chosen_limit;
          };
          with (chosen_count \in 0..sem_limit) {
              sem_count := chosen_count;
          };
          max_in_critical_section := sem_count;
          (* Assign to sem_initialized _after_ the marker label, as
           * this prevents a state explosion of other threads running
           * while we're still sitting at the marker label.*)
          if (sem_count = 0) {
            (* Need at least one token in the semaphore to prevent
             * deadlocking here.
             *)
            sem_init_forced_give:
              sem_init_state := SEM_INITIALIZED;
              if (max_in_critical_section < Cardinality(Proc)) {
                  max_in_critical_section := max_in_critical_section + 1;
              };
              goto sem_give;
          } else {
            sem_init_no_forced_give:
              sem_init_state := SEM_INITIALIZED;
              goto main_proc_start;
          };
      } or {
          (* Wait for other process to init semaphore.*)
          await sem_init_state = SEM_INITIALIZED;
        sem_other_process_init:
          goto main_proc_start;
      };
    main_proc_start:
      assert sem_count <= sem_limit;
      assert sem_count >= -Cardinality(Proc);
      either {
        (* Processes can drop out if they so choose, assuming
         * they don't have the semaphore.
         *)
        goto main_proc_done;
      } or {
        sem_take:
          either {
              sem_count := sem_count - 1;
              if (sem_count >= 0) {
                sem_take_success:
                  goto critical_section;
              } else {
                sem_take_add_to_sleepers:
                  sem_sleepers := sem_sleepers \union {self};
                sem_take_added_to_sleepers:
                  either {
                    sem_take_sleeping:
                      await ~ (self \in sem_sleepers);
                      goto critical_section;
                  } or {
                      await (self \in sem_sleepers);
                      sem_sleepers := sem_sleepers \ {self};
                    sem_take_failure_timed_out:
                      sem_count := sem_count + 1;
                      goto main_proc_start;
                  };
              };
          } or {
              if (sem_count <= 0) {
                sem_take_no_timeout_fail:
                  goto main_proc_start;
              } else {
                  sem_count := sem_count - 1;
                sem_take_no_timeout_success:
                  goto critical_section;
              };
          };
        critical_section:
          assert act_in_critical_section < max_in_critical_section;
          act_in_critical_section := act_in_critical_section + 1;
        critical_section_end:
          assert act_in_critical_section > 0;
          act_in_critical_section := act_in_critical_section - 1;
        sem_give:
          if (sem_count >= sem_limit) {
            sem_give_at_limit:
              goto main_proc_start;
          } else if (sem_count >= 0) {
              sem_count := sem_count + 1;
            sem_give_under_limit:
              goto main_proc_start;
          };
        sem_give_try_wake:
          if (sem_sleepers = {}) {
            sem_give_no_sleepers:
              goto sem_give;
          } else {
              with (x \in sem_sleepers) {
                  sem_sleepers := sem_sleepers \ {x};
              };
            sem_give_found_sleeper:
              sem_count := sem_count + 1;
              goto main_proc_start;
          };
      } or {
        main_proc_sem_reset:
          if (sem_count >= 0) {
              sem_count := 0;
            sem_reset_positive_count:
              if (max_in_critical_section < Cardinality(Proc)) {
                  max_in_critical_section := max_in_critical_section + 1;
              };
              goto sem_give;
          } else {
            sem_reset_wake_thread:
              if (sem_sleepers = {}) {
                sem_reset_no_thread_found:
                  goto main_proc_sem_reset;
              } else {
                  if (max_in_critical_section < Cardinality(Proc)) {
                      max_in_critical_section := max_in_critical_section + 1;
                  };
                  with (x \in sem_sleepers) {
                      sem_sleepers := sem_sleepers \ {x};
                  };
                sem_reset_atomic_inc:
                  sem_count := sem_count + 1;
                  goto main_proc_sem_reset;
              };
          };
      } or {
          (* Just add another token to the system. *)
          if (max_in_critical_section < Cardinality(Proc)) {
              max_in_critical_section := max_in_critical_section + 1;
          };
          goto sem_give;
      };
    (* We mark the processes as fair, which means that if
     * a step could be taken, it will be taken within a finite
     * number of steps. This is normally a fairly decent
     * assumption, assuming thread priorities are set up correctly.
     * However, in some cases we explicitly want to violate this
     * assumption, e.g. if a thread self-aborts. Handle that by
     * just skipping to Done.
     *)
    main_proc_done:
      skip;
  }
}
*)
\* BEGIN TRANSLATION (chksum(pcal) = "a64559fb" /\ chksum(tla) = "469c1ff6")
CONSTANT defaultInitValue
VARIABLES sem_init_state, sem_limit, max_in_critical_section,
          act_in_critical_section, sem_count, sem_sleepers, pc

vars == << sem_init_state, sem_limit, max_in_critical_section,
           act_in_critical_section, sem_count, sem_sleepers, pc >>

ProcSet == (Proc)

Init == (* Global variables *)
        /\ sem_init_state = SEM_NOT_INITIALIZED
        /\ sem_limit = defaultInitValue
        /\ max_in_critical_section = defaultInitValue
        /\ act_in_critical_section = 0
        /\ sem_count = defaultInitValue
        /\ sem_sleepers = defaultInitValue
        /\ pc = [self \in ProcSet |-> "maybe_init"]

maybe_init(self) == /\ pc[self] = "maybe_init"
                    /\ \/ /\ sem_init_state = SEM_NOT_INITIALIZED
                          /\ sem_init_state' = SEM_INITIALIZING
                          /\ pc' = [pc EXCEPT ![self] = "sem_init"]
                       \/ /\ sem_init_state = SEM_INITIALIZED
                          /\ pc' = [pc EXCEPT ![self] = "sem_other_process_init"]
                          /\ UNCHANGED sem_init_state
                    /\ UNCHANGED << sem_limit, max_in_critical_section,
                                    act_in_critical_section, sem_count,
                                    sem_sleepers >>

sem_init(self) == /\ pc[self] = "sem_init"
                  /\ sem_sleepers' = {}
                  /\ \E chosen_limit \in 1..MAX_SEM_LIMIT:
                       sem_limit' = chosen_limit
                  /\ \E chosen_count \in 0..sem_limit':
                       sem_count' = chosen_count
                  /\ max_in_critical_section' = sem_count'
                  /\ IF sem_count' = 0
                        THEN /\ pc' = [pc EXCEPT ![self] = "sem_init_forced_give"]
                        ELSE /\ pc' = [pc EXCEPT ![self] = "sem_init_no_forced_give"]
                  /\ UNCHANGED << sem_init_state, act_in_critical_section >>

sem_init_forced_give(self) == /\ pc[self] = "sem_init_forced_give"
                              /\ sem_init_state' = SEM_INITIALIZED
                              /\ IF max_in_critical_section < Cardinality(Proc)
                                    THEN /\ max_in_critical_section' = max_in_critical_section + 1
                                    ELSE /\ TRUE
                                         /\ UNCHANGED max_in_critical_section
                              /\ pc' = [pc EXCEPT ![self] = "sem_give"]
                              /\ UNCHANGED << sem_limit,
                                              act_in_critical_section,
                                              sem_count, sem_sleepers >>

sem_init_no_forced_give(self) == /\ pc[self] = "sem_init_no_forced_give"
                                 /\ sem_init_state' = SEM_INITIALIZED
                                 /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                                 /\ UNCHANGED << sem_limit,
                                                 max_in_critical_section,
                                                 act_in_critical_section,
                                                 sem_count, sem_sleepers >>

sem_other_process_init(self) == /\ pc[self] = "sem_other_process_init"
                                /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                                /\ UNCHANGED << sem_init_state, sem_limit,
                                                max_in_critical_section,
                                                act_in_critical_section,
                                                sem_count, sem_sleepers >>

main_proc_start(self) == /\ pc[self] = "main_proc_start"
                         /\ Assert(sem_count <= sem_limit,
                                   "Failure of assertion at line 103, column 7.")
                         /\ Assert(sem_count >= -Cardinality(Proc),
                                   "Failure of assertion at line 104, column 7.")
                         /\ \/ /\ pc' = [pc EXCEPT ![self] = "main_proc_done"]
                               /\ UNCHANGED max_in_critical_section
                            \/ /\ pc' = [pc EXCEPT ![self] = "sem_take"]
                               /\ UNCHANGED max_in_critical_section
                            \/ /\ pc' = [pc EXCEPT ![self] = "main_proc_sem_reset"]
                               /\ UNCHANGED max_in_critical_section
                            \/ /\ IF max_in_critical_section < Cardinality(Proc)
                                     THEN /\ max_in_critical_section' = max_in_critical_section + 1
                                     ELSE /\ TRUE
                                          /\ UNCHANGED max_in_critical_section
                               /\ pc' = [pc EXCEPT ![self] = "sem_give"]
                         /\ UNCHANGED << sem_init_state, sem_limit,
                                         act_in_critical_section, sem_count,
                                         sem_sleepers >>

sem_take(self) == /\ pc[self] = "sem_take"
                  /\ \/ /\ sem_count' = sem_count - 1
                        /\ IF sem_count' >= 0
                              THEN /\ pc' = [pc EXCEPT ![self] = "sem_take_success"]
                              ELSE /\ pc' = [pc EXCEPT ![self] = "sem_take_add_to_sleepers"]
                     \/ /\ IF sem_count <= 0
                              THEN /\ pc' = [pc EXCEPT ![self] = "sem_take_no_timeout_fail"]
                                   /\ UNCHANGED sem_count
                              ELSE /\ sem_count' = sem_count - 1
                                   /\ pc' = [pc EXCEPT ![self] = "sem_take_no_timeout_success"]
                  /\ UNCHANGED << sem_init_state, sem_limit,
                                  max_in_critical_section,
                                  act_in_critical_section, sem_sleepers >>

sem_take_success(self) == /\ pc[self] = "sem_take_success"
                          /\ pc' = [pc EXCEPT ![self] = "critical_section"]
                          /\ UNCHANGED << sem_init_state, sem_limit,
                                          max_in_critical_section,
                                          act_in_critical_section, sem_count,
                                          sem_sleepers >>

sem_take_add_to_sleepers(self) == /\ pc[self] = "sem_take_add_to_sleepers"
                                  /\ sem_sleepers' = (sem_sleepers \union {self})
                                  /\ pc' = [pc EXCEPT ![self] = "sem_take_added_to_sleepers"]
                                  /\ UNCHANGED << sem_init_state, sem_limit,
                                                  max_in_critical_section,
                                                  act_in_critical_section,
                                                  sem_count >>

sem_take_added_to_sleepers(self) == /\ pc[self] = "sem_take_added_to_sleepers"
                                    /\ \/ /\ pc' = [pc EXCEPT ![self] = "sem_take_sleeping"]
                                          /\ UNCHANGED sem_sleepers
                                       \/ /\ (self \in sem_sleepers)
                                          /\ sem_sleepers' = sem_sleepers \ {self}
                                          /\ pc' = [pc EXCEPT ![self] = "sem_take_failure_timed_out"]
                                    /\ UNCHANGED << sem_init_state, sem_limit,
                                                    max_in_critical_section,
                                                    act_in_critical_section,
                                                    sem_count >>

sem_take_sleeping(self) == /\ pc[self] = "sem_take_sleeping"
                           /\ ~ (self \in sem_sleepers)
                           /\ pc' = [pc EXCEPT ![self] = "critical_section"]
                           /\ UNCHANGED << sem_init_state, sem_limit,
                                           max_in_critical_section,
                                           act_in_critical_section, sem_count,
                                           sem_sleepers >>

sem_take_failure_timed_out(self) == /\ pc[self] = "sem_take_failure_timed_out"
                                    /\ sem_count' = sem_count + 1
                                    /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                                    /\ UNCHANGED << sem_init_state, sem_limit,
                                                    max_in_critical_section,
                                                    act_in_critical_section,
                                                    sem_sleepers >>

sem_take_no_timeout_fail(self) == /\ pc[self] = "sem_take_no_timeout_fail"
                                  /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                                  /\ UNCHANGED << sem_init_state, sem_limit,
                                                  max_in_critical_section,
                                                  act_in_critical_section,
                                                  sem_count, sem_sleepers >>

sem_take_no_timeout_success(self) == /\ pc[self] = "sem_take_no_timeout_success"
                                     /\ pc' = [pc EXCEPT ![self] = "critical_section"]
                                     /\ UNCHANGED << sem_init_state, sem_limit,
                                                     max_in_critical_section,
                                                     act_in_critical_section,
                                                     sem_count, sem_sleepers >>

critical_section(self) == /\ pc[self] = "critical_section"
                          /\ Assert(act_in_critical_section < max_in_critical_section,
                                    "Failure of assertion at line 144, column 11.")
                          /\ act_in_critical_section' = act_in_critical_section + 1
                          /\ pc' = [pc EXCEPT ![self] = "critical_section_end"]
                          /\ UNCHANGED << sem_init_state, sem_limit,
                                          max_in_critical_section, sem_count,
                                          sem_sleepers >>

critical_section_end(self) == /\ pc[self] = "critical_section_end"
                              /\ Assert(act_in_critical_section > 0,
                                        "Failure of assertion at line 147, column 11.")
                              /\ act_in_critical_section' = act_in_critical_section - 1
                              /\ pc' = [pc EXCEPT ![self] = "sem_give"]
                              /\ UNCHANGED << sem_init_state, sem_limit,
                                              max_in_critical_section,
                                              sem_count, sem_sleepers >>

sem_give(self) == /\ pc[self] = "sem_give"
                  /\ IF sem_count >= sem_limit
                        THEN /\ pc' = [pc EXCEPT ![self] = "sem_give_at_limit"]
                             /\ UNCHANGED sem_count
                        ELSE /\ IF sem_count >= 0
                                   THEN /\ sem_count' = sem_count + 1
                                        /\ pc' = [pc EXCEPT ![self] = "sem_give_under_limit"]
                                   ELSE /\ pc' = [pc EXCEPT ![self] = "sem_give_try_wake"]
                                        /\ UNCHANGED sem_count
                  /\ UNCHANGED << sem_init_state, sem_limit,
                                  max_in_critical_section,
                                  act_in_critical_section, sem_sleepers >>

sem_give_at_limit(self) == /\ pc[self] = "sem_give_at_limit"
                           /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                           /\ UNCHANGED << sem_init_state, sem_limit,
                                           max_in_critical_section,
                                           act_in_critical_section, sem_count,
                                           sem_sleepers >>

sem_give_under_limit(self) == /\ pc[self] = "sem_give_under_limit"
                              /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                              /\ UNCHANGED << sem_init_state, sem_limit,
                                              max_in_critical_section,
                                              act_in_critical_section,
                                              sem_count, sem_sleepers >>

sem_give_try_wake(self) == /\ pc[self] = "sem_give_try_wake"
                           /\ IF sem_sleepers = {}
                                 THEN /\ pc' = [pc EXCEPT ![self] = "sem_give_no_sleepers"]
                                      /\ UNCHANGED sem_sleepers
                                 ELSE /\ \E x \in sem_sleepers:
                                           sem_sleepers' = sem_sleepers \ {x}
                                      /\ pc' = [pc EXCEPT ![self] = "sem_give_found_sleeper"]
                           /\ UNCHANGED << sem_init_state, sem_limit,
                                           max_in_critical_section,
                                           act_in_critical_section, sem_count >>

sem_give_no_sleepers(self) == /\ pc[self] = "sem_give_no_sleepers"
                              /\ pc' = [pc EXCEPT ![self] = "sem_give"]
                              /\ UNCHANGED << sem_init_state, sem_limit,
                                              max_in_critical_section,
                                              act_in_critical_section,
                                              sem_count, sem_sleepers >>

sem_give_found_sleeper(self) == /\ pc[self] = "sem_give_found_sleeper"
                                /\ sem_count' = sem_count + 1
                                /\ pc' = [pc EXCEPT ![self] = "main_proc_start"]
                                /\ UNCHANGED << sem_init_state, sem_limit,
                                                max_in_critical_section,
                                                act_in_critical_section,
                                                sem_sleepers >>

main_proc_sem_reset(self) == /\ pc[self] = "main_proc_sem_reset"
                             /\ IF sem_count >= 0
                                   THEN /\ sem_count' = 0
                                        /\ pc' = [pc EXCEPT ![self] = "sem_reset_positive_count"]
                                   ELSE /\ pc' = [pc EXCEPT ![self] = "sem_reset_wake_thread"]
                                        /\ UNCHANGED sem_count
                             /\ UNCHANGED << sem_init_state, sem_limit,
                                             max_in_critical_section,
                                             act_in_critical_section,
                                             sem_sleepers >>

sem_reset_positive_count(self) == /\ pc[self] = "sem_reset_positive_count"
                                  /\ IF max_in_critical_section < Cardinality(Proc)
                                        THEN /\ max_in_critical_section' = max_in_critical_section + 1
                                        ELSE /\ TRUE
                                             /\ UNCHANGED max_in_critical_section
                                  /\ pc' = [pc EXCEPT ![self] = "sem_give"]
                                  /\ UNCHANGED << sem_init_state, sem_limit,
                                                  act_in_critical_section,
                                                  sem_count, sem_sleepers >>

sem_reset_wake_thread(self) == /\ pc[self] = "sem_reset_wake_thread"
                               /\ IF sem_sleepers = {}
                                     THEN /\ pc' = [pc EXCEPT ![self] = "sem_reset_no_thread_found"]
                                          /\ UNCHANGED << max_in_critical_section,
                                                          sem_sleepers >>
                                     ELSE /\ IF max_in_critical_section < Cardinality(Proc)
                                                THEN /\ max_in_critical_section' = max_in_critical_section + 1
                                                ELSE /\ TRUE
                                                     /\ UNCHANGED max_in_critical_section
                                          /\ \E x \in sem_sleepers:
                                               sem_sleepers' = sem_sleepers \ {x}
                                          /\ pc' = [pc EXCEPT ![self] = "sem_reset_atomic_inc"]
                               /\ UNCHANGED << sem_init_state, sem_limit,
                                               act_in_critical_section,
                                               sem_count >>

sem_reset_no_thread_found(self) == /\ pc[self] = "sem_reset_no_thread_found"
                                   /\ pc' = [pc EXCEPT ![self] = "main_proc_sem_reset"]
                                   /\ UNCHANGED << sem_init_state, sem_limit,
                                                   max_in_critical_section,
                                                   act_in_critical_section,
                                                   sem_count, sem_sleepers >>

sem_reset_atomic_inc(self) == /\ pc[self] = "sem_reset_atomic_inc"
                              /\ sem_count' = sem_count + 1
                              /\ pc' = [pc EXCEPT ![self] = "main_proc_sem_reset"]
                              /\ UNCHANGED << sem_init_state, sem_limit,
                                              max_in_critical_section,
                                              act_in_critical_section,
                                              sem_sleepers >>

main_proc_done(self) == /\ pc[self] = "main_proc_done"
                        /\ TRUE
                        /\ pc' = [pc EXCEPT ![self] = "Done"]
                        /\ UNCHANGED << sem_init_state, sem_limit,
                                        max_in_critical_section,
                                        act_in_critical_section, sem_count,
                                        sem_sleepers >>

P(self) == maybe_init(self) \/ sem_init(self) \/ sem_init_forced_give(self)
              \/ sem_init_no_forced_give(self)
              \/ sem_other_process_init(self) \/ main_proc_start(self)
              \/ sem_take(self) \/ sem_take_success(self)
              \/ sem_take_add_to_sleepers(self)
              \/ sem_take_added_to_sleepers(self)
              \/ sem_take_sleeping(self)
              \/ sem_take_failure_timed_out(self)
              \/ sem_take_no_timeout_fail(self)
              \/ sem_take_no_timeout_success(self)
              \/ critical_section(self) \/ critical_section_end(self)
              \/ sem_give(self) \/ sem_give_at_limit(self)
              \/ sem_give_under_limit(self) \/ sem_give_try_wake(self)
              \/ sem_give_no_sleepers(self) \/ sem_give_found_sleeper(self)
              \/ main_proc_sem_reset(self)
              \/ sem_reset_positive_count(self)
              \/ sem_reset_wake_thread(self)
              \/ sem_reset_no_thread_found(self)
              \/ sem_reset_atomic_inc(self) \/ main_proc_done(self)

(* Allow infinite stuttering to prevent deadlock on termination. *)
Terminating == /\ \A self \in ProcSet: pc[self] = "Done"
               /\ UNCHANGED vars

Next == (\E self \in Proc: P(self))
           \/ Terminating

Spec == /\ Init /\ [][Next]_vars
        /\ \A self \in Proc : WF_vars(P(self))

Termination == <>(\A self \in ProcSet: pc[self] = "Done")

\* END TRANSLATION
=====

