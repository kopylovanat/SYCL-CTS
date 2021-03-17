/*******************************************************************************
//
//  SYCL 2020 Conformance Test Suite
//
//  Copyright:	(c) 2018 by Codeplay Software LTD. All Rights Reserved.
//
*******************************************************************************/

#include "../common/common.h"

#define TEST_NAME hierarchical_private_memory

namespace TEST_NAMESPACE {

template <int dim>
class kernel;

using namespace sycl_cts;

template <int dim>
void check_dim(util::logger &log) {
  try {
    constexpr size_t globalRange1d = 6;
    constexpr size_t globalRange2d = 2;
    constexpr size_t globalRangeTotal = 24;
    constexpr size_t local = globalRange2d;
    std::vector<size_t> data(globalRangeTotal);

    auto myQueue = util::get_cts_object::queue();
    // using this scope we ensure that the buffer will update the host values
    // after the wait_and_throw
    {
      cl::sycl::buffer<size_t, 1> buf(
          data.data(), cl::sycl::range<1>(globalRangeTotal));

      myQueue.submit([&](cl::sycl::handler &cgh) {
        auto accessor =
            buf.template get_access<cl::sycl::access::mode::read_write>(cgh);
        auto globalRange =
          sycl_cts::util::get_cts_object::range<dim>::template
              get_fixed_size<globalRangeTotal>(globalRange1d, globalRange2d);
        auto localRange =
          sycl_cts::util::get_cts_object::range<dim>::get(local, local,
                                                      local);
        auto groupRange = globalRange / localRange;
        cgh.parallel_for_work_group<kernel<dim>>(
            groupRange, localRange, [=](cl::sycl::group<dim> group_pid) {
              cl::sycl::private_memory<size_t, dim> priv(group_pid);

              // Assign global linear id through private memory
              // as parallel_for_work_item scope is allocated in private memory
              group_pid.parallel_for_work_item(
                  [&](cl::sycl::h_item<dim> itemID) {
                    priv(itemID) = itemID.get_global().get_linear_id();
                  });

              group_pid.parallel_for_work_item([&](
                  cl::sycl::h_item<dim> itemID) {
                accessor[itemID.get_global().get_linear_id()] = priv(itemID);
              });
            });
      });
    }

    for (size_t i = 0; i < data.size(); i++) {
      if (data[i] != i) {
        cl::sycl::string_class errorMessage =
            cl::sycl::string_class("Value for global id ") +
            std::to_string(i) + cl::sycl::string_class(" was not correct (") +
            std::to_string(data[i]) + cl::sycl::string_class(" instead of ") +
            std::to_string(i) + ")";
        FAIL(log, errorMessage);
      }
    }

  } catch (const cl::sycl::exception &e) {
    log_exception(log, e);
    cl::sycl::string_class errorMsg =
        "a SYCL exception was caught: " + cl::sycl::string_class(e.what());
    FAIL(log, errorMsg.c_str());
  }
}
/** test cl::sycl::range::get(int index) return size_t
 */
class TEST_NAME : public util::test_base {
 public:
  /** return information about this test
   */
  void get_info(test_base::info &out) const override {
    set_test_info(out, TOSTRING(TEST_NAME), TEST_FILE);
  }

  /** execute the test
   */
  void run(util::logger &log) override {
    check_dim<1>(log);
    check_dim<2>(log);
    check_dim<3>(log);
  }
};

// construction of this proxy will register the above test
util::test_proxy<TEST_NAME> proxy;

}  // namespace TEST_NAMESPACE
