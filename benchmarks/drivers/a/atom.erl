-module(atom).
-export([main/0]).

%% Lado A - Atom table: insert (list_to_atom) + find (atom_to_binary) de N atoms.
%% RESULT = nomes ordenados byte-a-byte (paridade com qsort strcmp do lado B).
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Sorted} = timer:tc(fun() -> run(N) end),
    ab:emit(Sorted, TimeUs, 2 * N),
    halt(0).

run(N) ->
    Names = [<<"a", (integer_to_binary(I))/binary>> || I <- lists:seq(0, N - 1)],
    [list_to_atom(binary_to_list(Nm)) || Nm <- Names],
    lists:sort([atom_to_binary(list_to_atom(binary_to_list(Nm)), utf8) || Nm <- Names]).
