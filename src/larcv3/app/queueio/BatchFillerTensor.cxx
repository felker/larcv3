#ifndef __LARCV3THREADIO_BATCHFILLERTENSOR_CXX__
#define __LARCV3THREADIO_BATCHFILLERTENSOR_CXX__

#include "BatchFillerTensor.h"

#include <random>

namespace larcv3 {

  /// Global larcv3::BatchFillerTensorProcessFactory to register BatchFillerTensor
  static BatchFillerTensorProcessFactory<2>
      __global_BatchFillerTensor2DProcessFactory__;
  static BatchFillerTensorProcessFactory<3>
      __global_BatchFillerTensor3DProcessFactory__;

  template<size_t dimension>
  BatchFillerTensor<dimension>::BatchFillerTensor(const std::string name)
      : BatchFillerTemplate<float>(name) {}

  template<size_t dimension>
  void BatchFillerTensor<dimension>::configure(const PSet& cfg) {
    LARCV_DEBUG() << "start" << std::endl;
    _tensor_producer = cfg.get<std::string>("TensorProducer");
    _tensor_type = cfg.get<std::string>("TensorType", "sparse");

    if (_tensor_type != "dense" && _tensor_type != "sparse") {
      LARCV_CRITICAL() << "TensorType can only be dense or sparse." << std::endl;
      throw larbys();
    }

    _slice_v.clear();
    _slice_v.resize(1,0);
    _slice_v = cfg.get<std::vector<size_t> >("Channels", _slice_v);

    _voxel_base_value = cfg.get<float>("EmptyVoxelValue",0.);
    _allow_empty = cfg.get<bool>("AllowEmpty",false);

    LARCV_DEBUG() << "done" << std::endl;
  }

  template<size_t dimension>
  void BatchFillerTensor<dimension>::initialize() {}

  template<size_t dimension>
  void BatchFillerTensor<dimension>::_batch_begin_() {
    if(!batch_data().dim().empty() && (int)(batch_size()) != batch_data().dim().front()) {
      LARCV_INFO() << "Batch size changed " << batch_data().dim().front() << "=>" << batch_size() << std::endl;
      auto dim = batch_data().dim();
      dim[0] = batch_size();
      this->set_dim(dim);
    }
  }

  template<size_t dimension>
  void BatchFillerTensor<dimension>::_batch_end_() {
    if (logger().level() <= msg::kINFO)
      LARCV_INFO() << "Total data size: " << batch_data().data_size()
                   << std::endl;
  }

  template<size_t dimension>
  void BatchFillerTensor<dimension>::finalize() { _entry_data.clear(); }


  template<size_t dimension>
  size_t BatchFillerTensor<dimension>::_set_image_size(const EventTensor<dimension>& image_data) {

    auto const& image_v = image_data.as_vector();

    if (image_v.empty()) {
      LARCV_CRITICAL() << "Input image is empty!" << std::endl;
      throw larbys();
    }

    if (_slice_v.empty()) {
      _slice_v.resize(image_v.size());
      for (size_t i = 0; i < _slice_v.size(); ++i) {
        _slice_v.at(i) = i;
      }
    }

    _num_channels = _slice_v.size();
    _max_ch = 0;
    for (auto const& v : _slice_v) {
      if (_max_ch < v) {
        _max_ch = v;
      }
    }

    if (image_v.size() <= _max_ch) {
      LARCV_CRITICAL() << "Requested slice max channel (" << _max_ch
                       << ") exceeds available # of channels in the input Tensor" << std::endl;
      throw larbys();
    }

    _rows = image_v.at(_slice_v.front()).meta().rows();
    _cols = image_v.at(_slice_v.front()).meta().cols();

    LARCV_INFO() << "Rows = " << _rows << " ... Cols = " << _cols << std::endl;

    return (_rows * _cols * _num_channels);
  }

  template<size_t dimension>
  void BatchFillerTensor<dimension>::_assert_dimension(const EventTensor<dimension>& image_data) const {

    auto const& image_v = image_data.as_vector();

    if (_rows == kINVALID_SIZE) {
      LARCV_WARNING() << "_set_dimension() must be called prior to check_dimension()" << std::endl;
      return;
    }
    bool valid_ch   = (image_v.size() > _max_ch);
    bool valid_rows = true;
    for (size_t ch = 0; ch < _num_channels; ++ch) {
      size_t input_ch = _slice_v.at(ch);
      auto const& img = image_v.at(input_ch);

      valid_rows = (_rows == img.meta().rows());
      if (!valid_rows) {
        LARCV_ERROR() << "# of rows changed! (row,col): (" << _rows << "," << _cols << ") => ("
                      << img.meta().rows() << "," << img.meta().cols() << ")" << std::endl;
        break;
      }
    }

    bool valid_cols = true;
    for (size_t ch = 0; ch < _num_channels; ++ch) {
      size_t input_ch = _slice_v.at(ch);
      auto const& img = image_v.at(input_ch);
      valid_cols = ( _cols == img.meta().cols() );
      if (!valid_cols) {
        LARCV_ERROR() << "# of cols changed! (row,col): (" << _rows << "," << _cols << ") => ("
                      << img.meta().rows() << "," << img.meta().cols() << ")" << std::endl;
        break;
      }
    }
    if (!valid_rows) {
      LARCV_CRITICAL() << "# of rows in the input image have changed!" << std::endl;
      throw larbys();
    }
    if (!valid_cols) {
      LARCV_CRITICAL() << "# of cols in the input image have changed!" << std::endl;
      throw larbys();
    }
    if (!valid_ch) {
      LARCV_CRITICAL() << "# of channels have changed in the input image! Image vs. MaxCh ("
                       << image_v.size() << " vs. " << _max_ch << ")" << std::endl;
      throw larbys();
    }

  }


  template<size_t dimension>
  bool BatchFillerTensor<dimension>::process(IOManager& mgr) {

    LARCV_DEBUG() << "start" << std::endl;

    if (_tensor_type == "sparse") {
      return _process_sparse(mgr);
    }

    return _process_dense(mgr);
  }

  template<size_t dimension>
  bool BatchFillerTensor<dimension>::_process_dense(IOManager& mgr) {

    LARCV_DEBUG() << "start" << std::endl;

    if (dimension > 2) {
      LARCV_CRITICAL() << "BatchFillerTensor from dense Tensor only available in 2D." << std::endl;
      throw larbys();
    }

    auto const& image_data =
        mgr.get_data<larcv3::EventTensor<dimension>>(_tensor_producer);

    if (!_allow_empty && image_data.as_vector().empty()) {
      LARCV_CRITICAL() << "Could not locate non-empty voxel data w/ producer name "
                       << _tensor_producer << std::endl;
      throw larbys();
    }

    if (batch_data().dim().empty()) {
      _set_image_size(image_data);

      std::vector<int> dim;
      dim.resize(4);
      dim[0] = batch_size();
      dim[1] = _rows;
      dim[2] = _cols;
      dim[3] = _num_channels;
      set_dim(dim);
    } else {
      _assert_dimension(image_data);
    }

    if (_entry_data.size() != batch_data().entry_data_size()) {
      _entry_data.resize(batch_data().entry_data_size(), 0.);
    }

    for (auto& v : _entry_data) v = 0.;

    auto const& image_v = image_data.as_vector();

    for (size_t ch = 0; ch < _num_channels; ++ch) {

      size_t input_ch = _slice_v.at(ch);
      auto const& input_img = image_v.at(input_ch);

      auto const& input_image = input_img.as_vector();


      size_t idx = 0;
      for (size_t row = 0; row < _rows; ++row) {
        for (size_t col = 0; col < _cols; ++col) {
          _entry_data.at(idx * _num_channels + ch) = input_image.at(col * _rows + row);
          ++idx;
        }
      }
    }

    // record the entry data
    LARCV_INFO() << "Inserting entry data of size " << _entry_data.size()
                 << std::endl;
    set_entry_data(_entry_data);

    return true;

  }

  template<size_t dimension>
  bool BatchFillerTensor<dimension>::_process_sparse(IOManager& mgr) {
    auto const& voxel_data =
        mgr.get_data<larcv3::EventSparseTensor<dimension>>(_tensor_producer);
    if (!_allow_empty && voxel_data.as_vector().empty()) {
      LARCV_CRITICAL()
          << "Could not locate non-empty voxel data w/ producer name "
          << _tensor_producer << std::endl;
      throw larbys();
    }

    _num_channels = _slice_v.size();

    auto const& voxel_meta = voxel_data.as_vector().front().meta();
    // _rows = voxel_meta.rows();
    // _cols = voxel_meta.cols();
    std::vector<int> dim;
    dim.resize(dimension + 2);
    dim[0] = batch_size();
    for (size_t i = 1; i < dimension + 1; i++) {
      dim[i] = voxel_meta.number_of_voxels(i - 1);
    }
    dim[dimension + 1] = _num_channels;
    this->set_dim(dim);
    this->set_dense_dim(dim);


    if (_entry_data.size() != batch_data().entry_data_size())
      _entry_data.resize(batch_data().entry_data_size(), 0.);


    for (auto& v : _entry_data) v = _voxel_base_value;


    for ( auto const& voxel_set : voxel_data.as_vector()){
      auto & meta = voxel_set.meta();
      auto projection_id = meta.id();

      // Check that this projection ID is in the lists of channels:
      int count = _check_projection(projection_id);
      if (count < 0) continue;

      for (auto const& voxel : voxel_set.as_vector()) {
        _entry_data[voxel.id()] = voxel.value();
      }

      // Only read the first voxel set in 3D
      if (dimension == 3) {
        break;
      }
    }

    // record the entry data
    LARCV_INFO() << "Inserting entry data of size " << _entry_data.size()
                 << std::endl;
    set_entry_data(_entry_data);

    return true;
  }

  template<size_t dimension>
  int BatchFillerTensor<dimension>::_check_projection(const int & projection_id) {

    if (dimension == 3) {
      return 0;
    }

    bool found = false;
    int count = 0;
    for (auto & channel : _slice_v){
      if (channel == projection_id){
        found = true;
        break;
      }
      count ++;
    }

    if (found) {
      return count;
    } else {
      return -1;
    }
  }
}
#endif
