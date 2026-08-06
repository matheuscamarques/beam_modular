-module(loader).
-export([main/0]).

%% Side A - Pure parity: extracts atoms from the same .beam via beam_lib.
%% Usage: erl -noshell -pa <dir> -eval 'loader:main(), halt(0).' -extra <file.beam>
main() ->
    [Path | _] = init:get_plain_arguments(),
    case beam_lib:chunks(Path, [atoms]) of
        {ok, {_, [{atoms, Atoms}]}} ->
            {TimeUs, Lines} = timer:tc(fun() -> build_lines(Atoms) end),
            ab:emit(Lines, TimeUs, length(Atoms)),
            halt(0);
        Err ->
            io:format("ERROR ~p~n", [Err]),
            halt(1)
    end.

build_lines(Atoms) ->
    BinNames = [atom_to_binary(A, utf8) || A <- Atoms],
    Mod = hd(BinNames),
    [<<"module:", Mod/binary>> | [<<"atom:", N/binary>> || N <- BinNames]].
