// Copyright 2014 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef IMPALA_EXEC_DATA_SOURCE_SCAN_NODE_H_
#define IMPALA_EXEC_DATA_SOURCE_SCAN_NODE_H_

#include <string>
#include <boost/scoped_ptr.hpp>

#include "exec/data-source-row-converter.h"
#include "exec/scan-node.h"
#include "exec/external-data-source-executor.h"
#include "runtime/descriptors.h"
#include "runtime/mem-pool.h"

#include "gen-cpp/ExternalDataSource_types.h"

namespace impala {

class Tuple;

/// Scan node for external data sources. The external data source jar is loaded
/// in Prepare() (via an ExternalDataSourceExecutor), and then the data source
/// is called to receive row batches when necessary. This node converts the
/// rows stored in a thrift structure to RowBatches. The external data source is
/// closed in Close().
class DataSourceScanNode : public ScanNode {
 public:
  DataSourceScanNode(ObjectPool* pool, const TPlanNode& tnode, const DescriptorTbl& descs);

  ~DataSourceScanNode();

  /// Load the data source library and create the ExternalDataSourceExecutor.
  virtual Status Prepare(RuntimeState* state);

  /// Open the data source and initialize the first row batch.
  virtual Status Open(RuntimeState* state);

  /// Fill the next row batch, calls GetNext() on the external scanner.
  virtual Status GetNext(RuntimeState* state, RowBatch* row_batch, bool* eos);

  /// NYI
  virtual Status Reset(RuntimeState* state);

  /// Close the scanner, and report errors.
  virtual void Close(RuntimeState* state);

 protected:
  /// Write debug string of this into out.
  virtual void DebugString(int indentation_level, std::stringstream* out) const;

 private:
  /// Used to call the external data source.
  boost::scoped_ptr<ExternalDataSourceExecutor> data_source_executor_;

  /// Thrift structure describing the data source scan node.
  const TDataSourceScanNode data_src_node_;

  /// Descriptor of tuples read
  const TupleDescriptor* tuple_desc_;

  /// Tuple index in tuple row.
  int tuple_idx_;

  /// Current tuple.
  Tuple* tuple_;

  /// Vector containing slot descriptors for all materialized slots. These
  /// descriptors are sorted in order of increasing col_pos.
  /// TODO: Refactor to base class. HdfsScanNode has this and other nodes could use it.
  std::vector<SlotDescriptor*> materialized_slots_;

  /// The opaque handle returned by the data source for the scan.
  std::string scan_handle_;

  /// The current result from calling GetNext() on the data source. Contains the
  /// thrift representation of the rows.
  boost::scoped_ptr<extdatasource::TGetNextResult> input_batch_;

  /// A utility class used to materialize rows in a TRowBatch to tuples.
  boost::scoped_ptr<DataSourceRowConverter> row_converter_;

  /// Gets the next batch from the data source, stored in input_batch_.
  Status GetNextInputBatch();

  /// True if input_batch_ has more rows.
  bool InputBatchHasNext() {
    if (!input_batch_->__isset.rows) return false;
    return row_converter_->HasNextRow();
  }
};

}

#endif
