-module(msg).
-export([main/0]).

%% Lado A - Messaging: um processo envia N mensagens ao main (send/receive).
%% Paridade com mailbox enqueue/dequeue do lado B: sent=N, received=N.
main() ->
    [ArgN | _] = init:get_plain_arguments(),
    N = list_to_integer(ArgN),
    {TimeUs, Lines} = timer:tc(fun() -> run(N) end),
    ab:emit(Lines, TimeUs, 2 * N),
    halt(0).

run(N) ->
    Parent = self(),
    spawn(fun() -> sender(Parent, N) end),
    Received = receive_all(N, 0),
    [<<"sent=", (integer_to_binary(N))/binary>>,
     <<"received=", (integer_to_binary(Received))/binary>>].

sender(_P, 0) -> ok;
sender(P, N) -> P ! {m, N}, sender(P, N - 1).

receive_all(0, Acc) -> Acc;
receive_all(N, Acc) ->
    receive
        {m, _} -> receive_all(N - 1, Acc + 1)
    end.