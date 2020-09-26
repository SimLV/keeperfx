## High level

```
sequenceDiagram
    
loop each server tick
par
    Client1->>Server: PacketEx
and
    Client2->>Server: PacketEx
end
    Note over Server: Merging
    Server ->>Client1: PacketsEx × NumPlayers
    Server ->>Client2: PacketsEx × NumPlayers
    Note over Server: Processing
par
    Note over Client1: Merging
and
    Note over Client2: Merging
end

end
```

## Layers

Messy
| layer | targed desc | now state |
| bflib_network.c | medium level wrapper | now it is just trying to sync packets between server and all clients |
| internal network layer | not supported anymore | - |
| bflib_tcpsp.c | - | lowlevel wrappers (basically + just `readfrom client#`) |

## Low level



Maybe flow instead?
```
sequenceDiagram
Client ->> Client/packetList: set_packet_##
Client/packetList ->> Server/innerList: Server/LbNetwork_Exchange: Data
Server/innerList ->> Server/mergedList:
```

https://mermaid-js.github.io/mermaid-live-editor/