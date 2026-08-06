-module(ets).
-export([main/0]).

%% Lado A - ETS: insert N, lookup todas, delete pares, lookup das impares restantes.
%% RESULT: count final + found_all + found_rest (todos deterministicos).
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Lines} = timer:tc(fun() -> run(N) end),
    ab:emit(Lines, TimeUs, 3 * N),
    halt(0).

run(N) ->
    T = ets:new(bench, [set, public]),
    [ets:insert(T, {I, I * 2}) || I <- lists:seq(0, N - 1)],
    FoundAll = lists:sum(
        [case ets:lookup(T, I) of [{_, _}] -> 1; _ -> 0 end
         || I <- lists:seq(0, N - 1)]),
    [ets:delete(T, I) || I <- lists:seq(0, N - 1), I rem 2 =:= 0],
    Count = ets:info(T, size),
    FoundRest = lists:sum(
        [case ets:lookup(T, I) of [{_, _}] -> 1; _ -> 0 end
         || I <- lists:seq(1, N - 1, 2)]),
    [list_to_binary(io_lib:format("count=~p", [Count])),
     list_to_binary(io_lib:format("found_all=~p", [FoundAll])),
     list_to_binary(io_lib:format("found_rest=~p", [FoundRest]))].
