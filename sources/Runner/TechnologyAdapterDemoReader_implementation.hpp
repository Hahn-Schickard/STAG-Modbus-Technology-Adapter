#include <Information_Model/mocks/DeviceMockBuilder.hpp>
#include <Technology_Adapter_Interface/mocks/ModelRepositoryInterface_MOCK.hpp>

namespace Technology_Adapter::Demo_Reader {

template <class TechnologyAdapterImplementation>
DemoReader::DemoReader(TypeInfo<TechnologyAdapterImplementation>,
    ConstString::ConstString const& config_path)
    : readables_(NonemptyPointer::NonemptyPtr<
          Threadsafe::SharedPtr<Readables>>::make()),
      adapter_(NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<
              TechnologyAdapterImplementation>>::make(config_path.c_str())) {

  /*
    We let lambdas capture this smart pointer so we are on the safe side w.r.t.
    lifetimes. Otherwise we might have just let them capture `this` and made
    the `registrate` overloads non-`static`.
  */
  auto readables = readables_;

  adapter_->setInterfaces( //
      NonemptyPointer::make_shared<
          Information_Model::testing::DeviceMockBuilder>(),
      NonemptyPointer::make_shared<::testing::NiceMock<
          Technology_Adapter::testing::ModelRepositoryMock>>(
          [readables](Information_Model::NonemptyDevicePtr const& device) {
            registrate(*readables, device);
            return true;
          },
          [readables](std::string const& device_id) {
            std::cout << "Deregistering device " << device_id << std::endl;
            readables->remove(ConstString::ConstString(device_id));
            return true;
          }));
}

} // namespace Technology_Adapter::Demo_Reader
