#include "dummy_legion.hh"

#include <cstddef>
#include <vector>

namespace Legion {

/* Geometric types. */

template <unsigned int DIM, typename T>
Point<DIM, T>::Point(T p) {
    coords.push_back(p);
}
template <unsigned int DIM, typename T>
Point<DIM, T>::Point(T p1, T p2) {
    coords.push_back(p1);
    coords.push_back(p2);
}
template <unsigned int DIM, typename T>
template <unsigned int DIM2, typename T2>
bool Point<DIM, T>::operator==(const Point<DIM2, T2>& other) {
    if (DIM != DIM2) {
        return false;
    }
    for (unsigned int i = 0; i < DIM; i++) {
        if (this->coords[i] != other.coords[i]) {
            return false;
        }
    }
    return true;
}
template <unsigned int DIM, typename T>
T& Point<DIM, T>::operator[](unsigned int ix) {
    return coords[ix];
}

template <unsigned int DIM>
DomainPoint::DomainPoint(const Point<DIM>& rhs) {
    for (size_t i = 0; i < DIM; i++) {
        coords.push_back(rhs[i]);
    }
}
DomainPoint::DomainPoint(coord_t coord) { coords.push_back(coord); }

template <unsigned int DIM, typename T>
Rect<DIM, T>::Rect(Point<DIM, T> lo_, Point<DIM, T> hi_) {
    lo = lo_;
    hi = hi_;
}

template <unsigned int DIM, typename T>
Domain::Domain(const Rect<DIM, T>& other) {
    lo = other.lo;
    hi = other.hi;
}

template <unsigned int DIM, typename T>
PointInRectIterator<DIM, T>::PointInRectIterator(const Rect<DIM, T>& r,
                                                 bool column_major_order) {
    start = r.lo;
    cur = start;
    end = r.hi;
    col_major = column_major_order;
}

template <unsigned int DIM, typename T>
bool PointInRectIterator<DIM, T>::operator()(void) const {
    return cur != end;
}
template <unsigned int DIM, typename T>
Point<DIM, T> PointInRectIterator<DIM, T>::operator*(void)const {
    return &cur;
}
template <unsigned int DIM, typename T>
PointInRectIterator<DIM, T> PointInRectIterator<DIM, T>::operator++(int) {
    if (col_major) {
        for (unsigned int i = 0; i < DIM; i++) {
            if (cur[i] == end[i]) {
                cur[i] = start[i];
            } else {
                cur[i] = cur[i] + 1;
                break;
            }
        }
    } else {
        for (unsigned int i = DIM - 1; 0 <= i; i--) {
            if (cur[i] == end[i]) {
                cur[i] = start[i];
            } else {
                cur[i] = cur[i] + 1;
                break;
            }
        }
    }
    return *this;
}

/* Memory structures. */

template <unsigned int DIM>
IndexSpaceT<DIM>::IndexSpaceT(const IndexSpace& rhs) {}

FieldID FieldAllocator::allocate_field(size_t field_size,
                                       FieldID desired_fieldid) {}

template <unsigned int DIM>
LogicalRegionT<DIM>::LogicalRegionT(const LogicalRegion& rhs) {}

RegionRequirement::RegionRequirement(LogicalRegion _handle,
                                     PrivilegeMode _priv,
                                     CoherenceProperty _prop,
                                     LogicalRegion _parent) {}
RegionRequirement& RegionRequirement::add_field(FieldID fid) {}

template <PrivilegeMode MODE, typename FT, int N>
FieldAccessor<MODE, FT, N>::FieldAccessor(const PhysicalRegion& region,
                                          FieldID fid) {}
template <PrivilegeMode MODE, typename FT, int N>
FT& FieldAccessor<MODE, FT, N>::operator[](const Point<N>&) const {}

/* Runtime types and classes. */

template <typename T>
T Future::get_result() const {}
bool Future::is_ready() const {}

ProcessorConstraint::ProcessorConstraint(Processor::Kind kind) {}

TaskArgument::TaskArgument(const void* arg, size_t argsize) {}

TaskLauncher::TaskLauncher(TaskID tid, TaskArgument arg) {}
RegionRequirement& TaskLauncher::add_region_requirement(
    const RegionRequirement& req) {}
void TaskLauncher::add_field(unsigned int idx, FieldID fid) {}

TaskVariantRegistrar::TaskVariantRegistrar(TaskID task_id,
                                           const char* variant_name) {}
TaskVariantRegistrar& TaskVariantRegistrar::add_constraint(
    const ProcessorConstraint& constraint) {}

InlineLauncher::InlineLauncher(const RegionRequirement& req) {}

InputArgs Runtime::get_input_args() {}
void Runtime::set_top_level_task_id(TaskID top_id) {}
int Runtime::start(int argc, char** argv) {}
IndexSpace Runtime::create_index_space(Context ctx, const Domain& bounds) {}
IndexPartition Runtime::create_equal_partition(Context ctx, IndexSpace parent,
                                               IndexSpace color_space) {}
FieldSpace Runtime::create_field_space(Context ctx) {}
FieldAllocator Runtime::create_field_allocator(Context ctx,
                                               FieldSpace handle) {}
LogicalRegion Runtime::create_logical_region(Context ctx, IndexSpace index,
                                             FieldSpace fields) {}
PhysicalRegion Runtime::map_region(Context ctx,
                                   const InlineLauncher& launcher) {}
void Runtime::unmap_region(Context ctx, PhysicalRegion region) {}
LogicalPartition Runtime::get_logical_partition(LogicalRegion parent,
                                                IndexPartition handle) {}
LogicalRegion Runtime::get_logical_subregion_by_color(LogicalPartition parent,
                                                      const DomainPoint& c) {}
Future Runtime::execute_task(Context ctx, const TaskLauncher& launcher) {}

}  // namespace Legion
