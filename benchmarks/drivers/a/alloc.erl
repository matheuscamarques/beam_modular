-module(alloc).
-export([main/0]).

%% Lado A - Allocator: ciclos de alocacao/desalocacao com padrao deterministico
%% de tamanhos: size = (i rem 5) * 16 + 4 bytes, tocando todos os bytes.
%% RESULT: ops + total de bytes somados (paridade aritmetica com o lado B).
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Lines} = timer:tc(fun() -> run(N) end),
    ab:emit(Lines, TimeUs, N),
    halt(0).

run(N) ->
    Total = lists:foldl(
        fun(I, Acc) ->
            Size = (I rem 5) * 16 + 4,
            B = binary:copy(<<0>>, Size),
            Acc + size(B)
        end, 0, lists:seq(1, N)),
    [<<"ops=", (integer_to_binary(N))/binary>>,
     <<"total_used=", (integer_to_binary(Total))/binary>>].