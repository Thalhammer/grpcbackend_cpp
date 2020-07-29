protoc-gen-jsonrpc
=========
Protobuf compiler plugin to generate JsonRPC apis based on Protobuf message definitions.
It is designed to support multiple different target with the following being currently implemented:
* grpcbackend_cpp Server
* TypeScript Client

Build
-----
Gets built along with grpcbackend, but does not depend on it directly.

Dependencies
------------
libprotobuf-dev

libprotoc-dev

Usage
-----
Example given:

Generate a typescript client based on the api defined in rpc_api.proto
```
protoc --proto_path=. --jsonrpc_out=out --jsonrpc_opt=typescript_client rpc_api.proto
```

Generate a grpcbackend_cpp Serverstub based on the api defined in rpc_api.proto
```
protoc --proto_path=. --jsonrpc_out=out rpc_api.proto
```