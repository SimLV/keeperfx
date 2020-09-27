## High level

```mermaid
sequenceDiagram
    
loop each server tick
par
    Client1->>Server: PacketEx<br> unconfirmed actions
and
    Client2->>Server: PacketEx<br> unconfirmed actions
end
    Note over Server: Merging events<br>discarding impossible events
    Server ->>Client1: PacketsEx × NumPlayers
    Server ->>Client2: PacketsEx × NumPlayers
    Note over Server: Processing
par
    Note over Client1: Removing confirmed actions
    Note over Client1: Merging
and
    Note over Client2: Removing confirmed actions
    Note over Client2: Merging
end

end
```

## Layers

Logical layers are a bit messy now.
Each layer should abstract away some specific problem.

| layer                  | targed desc                | now state                        |
| ---------------------- |  -----                     |   --------------------------                                         |
| packets.c              | make_packet/process_packet |  now it is processing packets synced by next layer                   |
| bflib_network.c        | medium level wrapper       | it is just trying to sync packets between server and all clients     |
| internal network layer | not supported anymore      | some code about IPX, Modem etc                                       |
| bflib_tcpsp.c          | -----                      | lowlevel wrappers (basically + just `readfrom client#`)              |

## Low level

TODO: this part should be implemented only when layer are clarifyed

```mermaid
sequenceDiagram
Client ->> Client/packetList: set_packet_##
Client/packetList ->> Server/innerList: Server/LbNetwork_Exchange: 
Server/innerList ->> Server/mergedList: merging
Server/mergedList ->> Client/packetList: updated list of all players
```

https://mermaid-js.github.io/mermaid-live-editor/
