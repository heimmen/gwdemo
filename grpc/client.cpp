#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "simple_app.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using simpleapp::HelloRequest;
using simpleapp::HelloResponse;
using simpleapp::SimpleService;

class SimpleClient {
 public:
  SimpleClient(std::shared_ptr<Channel> channel) : stub_(SimpleService::NewStub(channel)) {}

  std::string SayHello(const std::string& user) {
    HelloRequest request;
    request.set_name(user);

    HelloResponse response;
    ClientContext context;

    Status status = stub_->SayHello(&context, request, &response);

    if (status.ok()) {
      return response.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<SimpleService::Stub> stub_;
};

int main(int argc, char** argv) {
  SimpleClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = client.SayHello(user);
  std::cout << "Client received: " << reply << std::endl;

  return 0;
}