#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "simple_app.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using simpleapp::HelloRequest;
using simpleapp::HelloResponse;
using simpleapp::SimpleService;

// 服务实现
class SimpleServiceImpl final : public SimpleService::Service {
  Status SayHello(ServerContext* context, const HelloRequest* request, HelloResponse* response) override {
    std::string prefix("Hello ");
    response->set_message(prefix + request->name());
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  SimpleServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}