-module(emu_loop).
-export([main/0]).

%% Lado A - Piloto interpretador: recursao de cauda aritmética (loop(N-1, Acc+1)).
%% RESULT: valor final (deve bater com o loop ADD/CALL do interpretador em B).
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Value} = timer:tc(fun() -> run(N) end),
    ab:emit([<<"value=", (integer_to_binary(Value))/binary>>], TimeUs, N),
    halt(0).

run(N) -> loop(N, 0).

loop(0, Acc) -> Acc;
loop(N, Acc) -> loop(N - 1, Acc + 1).