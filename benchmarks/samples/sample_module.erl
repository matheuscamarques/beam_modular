-module(sample_module).
-export([hello/1, loop/2, compute/1]).

hello(Name) ->
    io:format("hello ~s~n", [Name]).

loop(0, Acc) -> Acc;
loop(N, Acc) -> loop(N - 1, Acc + 1).

compute(List) -> lists:sum(List).