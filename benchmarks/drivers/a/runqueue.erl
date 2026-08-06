-module(runqueue).
-export([main/0]).

%% Side A - Run queue proxy: spawn N processes with a mix of priorities
%% (max/high/normal/low) that complete trivial work and report 'done'.
%% NOTE: semantic proxy - the Erlang scheduler has no visible enqueue/dequeue;
%% metrics are time to complete N procs + reductions/s.
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Lines} = timer:tc(fun() -> run(N) end),
    ab:emit(Lines, TimeUs, 2 * N),
    halt(0).

run(N) ->
    Parent = self(),
    [spawn_opt(fun() -> Parent ! done end, [{priority, prio(I)}])
     || I <- lists:seq(1, N)],
    Done = collect(N, 0),
    [<<"spawned=", (integer_to_binary(N))/binary>>,
     <<"done=", (integer_to_binary(Done))/binary>>].

prio(I) -> element((I rem 4) + 1, {max, high, normal, low}).

collect(0, Acc) -> Acc;
collect(N, Acc) ->
    receive
        done -> collect(N - 1, Acc + 1)
    end.