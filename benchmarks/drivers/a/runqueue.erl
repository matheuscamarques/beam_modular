-module(runqueue).
-export([main/0]).

%% Lado A - Proxy de run queue: spawn de N processos com mix de prioridades
%% (max/high/normal/low) que completam trabalho trivial e reportam 'done'.
%% NOTA: proxy semantico - o scheduler Erlang nao tem enqueue/dequeue visivel;
%% metricas sao tempo para completar N procs + reductions/s.
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