#ifndef MK_DAOS_ZIPKIN_LOG_H
#define MK_DAOS_ZIPKIN_LOG_H

#include "interfaces.h"
#include <opentelemetry/exporters/zipkin/zipkin_exporter.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span_metadata.h>

#include "mocking.h"
#include <string>
#include <iostream>

namespace trace = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace zipkin = opentelemetry::exporter::zipkin;
namespace resource = opentelemetry::sdk::resource;

void init_opentelemetry() {
	zipkin::ZipkinExporterOptions opts;
	std::cout << "-- Pushing events to: " << opts.endpoint << std::endl;

	// Create zipkin exporter instance
	resource::ResourceAttributes attributes = {{"service.name", "daos-service"}};

	auto resource = resource::Resource::Create(attributes);
	auto exporter = std::unique_ptr<trace_sdk::SpanExporter>(
		new zipkin::ZipkinExporter(opts));

	auto batch_procesor_options =
		opentelemetry::sdk::trace::BatchSpanProcessorOptions();
	batch_procesor_options.max_export_batch_size = 1024 * 8;
	batch_procesor_options.max_queue_size = 1024 * 4;

	auto processor = std::unique_ptr<trace_sdk::SpanProcessor>(
		new trace_sdk::BatchSpanProcessor(std::move(exporter),
										  batch_procesor_options));

	auto provider = nostd::shared_ptr<trace::TracerProvider>(
		new trace_sdk::TracerProvider(std::move(processor), resource));

	// Set the global trace provider
		trace::Provider::SetTracerProvider(provider);
}

class DAOSZipkinLog : public IDAOSLog {

 public:

  DAOSZipkinLog(): scope_(get_tracer()->StartSpan("root")) {}

  nostd::shared_ptr<trace_api::Tracer> get_tracer() {
		auto provider = trace_api::Provider::GetTracerProvider();
		return provider->GetTracer("daos-mock");
  }

  void log(const std::string& func_name,
		   MockDAOSOperationType operation = UNDEFINED) {

//	auto span = get_tracer()->StartSpan(func_name);
//	span->SetAttribute("Type", (int)operation);
//	span->SetAttribute("Function", func_name);
//	span->End();


	 switch (operation) {
		  case UNDEFINED:{
					auto span = get_tracer()->StartSpan("Undefined");
					span->SetAttribute("Function", func_name);
					span->End();
				}
				break;
		  case POOL_CONNECT: 
				pool_span_ = get_tracer()->StartSpan("Pool connnected");
				break;
			case CONT_CREATE:
				container_created_span_ = get_tracer()->StartSpan("Container created");
				break;
		  case CONT_DESTROY: 
				container_created_span_->End();
				break;
		  case CONT_OPEN: 
				container_opened_span_ = get_tracer()->StartSpan("Container opened");
				break;
		  case CONT_CLOSE: 
				if(container_opened_span_){
					container_opened_span_->End();
				} else {
					std::cerr << "Container closed before opening" << std::endl;
				}
			  break;
		  case KV_GET:{
					auto span = get_tracer()->StartSpan("Get");
					span->SetAttribute("Function", func_name);
					span->End();
				}
				break;
		  case KV_PUT:{
					auto span = get_tracer()->StartSpan("Put");
					span->SetAttribute("Function", func_name);
					span->End();
				}
				break;
		  case KV_OPEN:{
					auto span = get_tracer()->StartSpan("Open");
					span->SetAttribute("Function", func_name);
					span->End();
				}
				break;
		  default:
			throw std::runtime_error("Bad enumeration in MockDAOSOperationType");
		}
  }

 private:
	trace_api::Scope scope_;


	std::shared_ptr<trace::Span> container_created_span_;
	std::shared_ptr<trace::Span> container_opened_span_;
	std::shared_ptr<trace::Span> pool_span_;


  std::string zipkin_instance_url_;
};

#endif
