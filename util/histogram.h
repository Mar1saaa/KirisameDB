#ifndef STORAGE_UTIL_HISTOGRAM_H_
#define STORAGE_UTIL_HISTOGRAM_H_

#include <string>

namespace kirisamedb {

class Histogram {
 public:
  Histogram() {}
  ~Histogram() {}

  void Clear();
  void Add(double value);
  void Merge(const Histogram& other);

  std::string ToString() const;

 private:
  enum { kNumBuckets = 154 };

  double Median() const;
  double Percentile(double p) const;
  double Average() const;
  double StandardDeviation() const;

  static const double kBucketLimit[kNumBuckets];

  double min_;
  double max_;
  double num_;
  double sum_;
  double sum_squares_;

  double buckets_[kNumBuckets];
};

}  // namespace kirisamedb

#endif  // STORAGE_UTIL_HISTOGRAM_H_