-module(ab).
-export([fnv1a/1, emit/3]).

%% FNV-1a 64-bit (mesmo algoritmo do lado C dos drivers B)
fnv1a(Bin) ->
    fnv1a(Bin, 16#cbf29ce484222325).

fnv1a(<<>>, H) ->
    H band 16#FFFFFFFFFFFFFFFF;
fnv1a(<<B, Rest/binary>>, H) ->
    fnv1a(Rest, ((H bxor B) * 16#100000001b3) band 16#FFFFFFFFFFFFFFFF).

%% Protocolo de saida comum (paridade byte-a-byte com os drivers C):
%%   RESULT <linha>   (repetido, na ordem canonica)
%%   FINGERPRINT <hex64> (FNV-1a sobre a concatenacao das linhas + "\n")
%%   TIME_US <int>
%%   OPS <int>
emit(Lines, TimeUs, Ops) ->
    [io:format("RESULT ~s~n", [L]) || L <- Lines],
    Fingerprint = fnv1a(list_to_binary([<<L/binary, "\n">> || L <- Lines])),
    io:format("FINGERPRINT ~16.16.0b~n", [Fingerprint]),
    io:format("TIME_US ~p~n", [TimeUs]),
    io:format("OPS ~p~n", [Ops]).
