/* Template and inline function definitions. */
#ifndef SERIAL_LEGION_INL_HH_
#define SERIAL_LEGION_INL_HH_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "serial_legion.hh"

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
T& Point<DIM, T>::operator[](unsigned int ix) {
    return coords.at(ix);
}
template <unsigned int DIM, typename T>
const T& Point<DIM, T>::operator[](unsigned int ix) const {
    return coords.at(ix);
}

template <unsigned int DIM, typename T>
Point<DIM, T>::operator T() const {
    if (coords.size() == 1) {
        return coords.at(0);
    } else {
        throw std::logic_error(
            "cannot to cast multi-dimensional point to value type");
    }
}

template <unsigned int DIM>
DomainPoint::DomainPoint(const Point<DIM>& rhs) {
    for (size_t i = 0; i < DIM; i++) {
        coords.push_back(rhs[i]);
    }
}
inline DomainPoint::DomainPoint(coord_t coord) { coords.push_back(coord); }
inline bool DomainPoint::operator==(const DomainPoint& other) const {
    return coords == other.coords;
}
inline coord_t& DomainPoint::operator[](unsigned int ix) {
    return coords.at(ix);
}
inline const coord_t& DomainPoint::operator[](unsigned int ix) const {
    return coords.at(ix);
}

template <unsigned int DIM, typename T>
Rect<DIM, T>::Rect(Point<DIM, T> lo_, Point<DIM, T> hi_) : lo(lo_), hi(hi_) {}

template <unsigned int DIM, typename T>
Domain::Domain(const Rect<DIM, T>& other) : lo(other.lo), hi(other.hi) {}
inline bool Domain::operator==(const Domain& other) const {
    return lo == other.lo && hi == other.hi;
}
inline size_t Domain::size() const {
    size_t size = 1;
    for (unsigned int i = 0; i < lo.coords.size(); i++) {
        size *= hi.coords.at(i) - lo.coords.at(i) + 1;
    }
    return size;
}

template <unsigned int DIM, typename T>
PointInRectIterator<DIM, T>::PointInRectIterator(const Rect<DIM, T>& r,
                                                 bool column_major_order)
    : start(r.lo), cur(r.lo), end(r.hi), col_major(column_major_order) {}

template <unsigned int DIM, typename T>
bool PointInRectIterator<DIM, T>::operator()(void) const {
    return cur.coords != end.coords;
}
template <unsigned int DIM, typename T>
Point<DIM, T> PointInRectIterator<DIM, T>::operator*(void) const {
    return cur;
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

inline IndexSpace::IndexSpace(Domain _dom) : dom(_dom) {}
inline bool IndexSpace::operator==(const IndexSpace& other) const {
    return dom == other.dom;
}
inline size_t IndexSpace::size() const { return dom.size(); }
template <unsigned int DIM>
IndexSpaceT<DIM>::IndexSpaceT(const IndexSpace& rhs) : IndexSpace(rhs.dom) {}

inline FieldSpace::FieldSpace(FieldSpaceID fsid) : id(fsid) {}
inline bool FieldSpace::operator==(const FieldSpace& other) const {
    return id == other.id;
}

inline FieldAllocator::FieldAllocator(FieldSpaceID _id) : id(_id) {}
inline FieldID FieldAllocator::allocate_field(size_t field_size,
                                              FieldID desired_fieldid) {
    Runtime::field_spaces.at(id).insert_or_assign(desired_fieldid, field_size);
    return desired_fieldid;
}

inline LogicalRegion::LogicalRegion(RegionID _id) : id(_id) {}
inline bool LogicalRegion::operator==(const LogicalRegion& other) const {
    return id == other.id;
}
template <unsigned int DIM>
LogicalRegionT<DIM>::LogicalRegionT(const LogicalRegion& rhs)
    : LogicalRegion(rhs.id) {}
inline LogicalPartition::LogicalPartition(LogicalRegion _region)
    : region(_region) {}

inline RegionRequirement::RegionRequirement(LogicalRegion _handle,
                                            PrivilegeMode _priv,
                                            CoherenceProperty _prop,
                                            LogicalRegion _parent)
    : region(_handle) {}
inline RegionRequirement& RegionRequirement::add_field(FieldID fid) {
    field_ids.push_back(fid);
    return *this;
}

inline PhysicalRegion::PhysicalRegion(RegionID _id) : id(_id) {}

template <PrivilegeMode MODE, typename FT, int N>
FieldAccessor<MODE, FT, N>::FieldAccessor(const PhysicalRegion& region,
                                          FieldID fid)
    : store(region), field(fid) {}
template <PrivilegeMode MODE, typename FT, int N>
FT& FieldAccessor<MODE, FT, N>::operator[](const Point<N>& p) const {
    Domain dom = Runtime::logical_regions.at(store.id).first.dom;
    size_t index = 0;
    size_t dim_prod = 1;
    for (unsigned int dim = 0; dim < p.coords.size(); dim++) {
        index += p[dim] * dim_prod;
        dim_prod *= dom.hi[dim] - dom.lo[dim] + 1;
    }
    uint8_t* base = static_cast<uint8_t*>(
        Runtime::physical_regions.at(store.id).at(field));
    size_t fsize = Runtime::field_spaces
                       .at(Runtime::logical_regions.at(store.id).second.id)
                       .at(field);
    return *(FT*)(base + index * fsize);
}

/* Runtime types and classes. */

inline Future::Future(void* _res) : res(_res) {}
template <typename T>
T Future::get_result() const {
    return *(T*)res;
}
inline bool Future::is_ready() const { return true; }

inline ProcessorConstraint::ProcessorConstraint(Processor::Kind kind) {}

inline TaskArgument::TaskArgument(const void* arg, size_t argsize)
    : _arg(arg), _argsize(argsize) {}
inline Task::Task(TaskArgument ta) {
    args = std::malloc(ta._argsize);
    memcpy(args, ta._arg, ta._argsize);
}
inline Task::~Task() { std::free(args); }
inline TaskLauncher::TaskLauncher(TaskID tid, TaskArgument arg)
    : _tid(tid), _arg(arg) {}
inline RegionRequirement& TaskLauncher::add_region_requirement(
    const RegionRequirement& req) {
    reqs.push_back(req);
    return reqs.at(reqs.size() - 1);
}
inline void TaskLauncher::add_field(unsigned int idx, FieldID fid) {
    reqs.at(idx).add_field(fid);
}

inline TaskVariantRegistrar::TaskVariantRegistrar(TaskID task_id,
                                                  const char* variant_name)
    : id(task_id) {}
inline TaskVariantRegistrar& TaskVariantRegistrar::add_constraint(
    const ProcessorConstraint& constraint) {
    return *this;
}

inline InlineLauncher::InlineLauncher(const RegionRequirement& req)
    : _req(req) {}

inline InputArgs Runtime::get_input_args() { return input_args; }
inline void Runtime::set_top_level_task_id(TaskID top_id) {
    top_level_task_id = top_id;
}
inline int Runtime::start(int argc, char** argv) {
    input_args = {.argc = argc, .argv = argv};
    Task task(TaskArgument(nullptr, 0));
    Runtime rt;
    tasks.at(top_level_task_id)
        ->run(&task, std::vector<PhysicalRegion>(), Context(), &rt);

    // Free dynamically allocated memory.
    for (auto future : futures) {
        std::free(future.res);
    }
    for (auto task : tasks) {
        delete task.second;
    }
    for (auto region : physical_regions) {
        for (auto field : region) {
            std::free(field.second);
        }
    }

    return 0;
}
inline IndexSpace Runtime::create_index_space(Context ctx,
                                              const Domain& bounds) {
    return IndexSpace(bounds);
}
inline void Runtime::destroy_index_space(Context ctx, IndexSpace handle) {
    return;
}
inline IndexPartition Runtime::create_equal_partition(Context ctx,
                                                      IndexSpace parent,
                                                      IndexSpace color_space) {
    return IndexPartition();
}
inline FieldSpace Runtime::create_field_space(Context ctx) {
    field_spaces.emplace_back();
    return FieldSpace(field_spaces.size() - 1);
}
inline void Runtime::destroy_field_space(Context ctx, FieldSpace handle) {
    field_spaces.erase(field_spaces.begin() + handle.id);
}
inline FieldAllocator Runtime::create_field_allocator(Context ctx,
                                                      FieldSpace handle) {
    return FieldAllocator(handle.id);
}
inline LogicalRegion Runtime::create_logical_region(Context ctx,
                                                    IndexSpace index,
                                                    FieldSpace fields) {
    RegionID id = logical_regions.size();
    logical_regions.push_back(std::make_pair(index, fields));
    // Allocate storage space.
    physical_regions.emplace_back();
    for (auto field : field_spaces.at(fields.id)) {
        physical_regions.at(id).insert_or_assign(
            field.first, std::malloc(field.second * index.size()));
    }
    return LogicalRegion(id);
}
inline void Runtime::destroy_logical_region(Context ctx,
                                            LogicalRegion handle) {
    logical_regions.erase(logical_regions.begin() + handle.id);
}
inline PhysicalRegion Runtime::map_region(Context ctx,
                                          const InlineLauncher& launcher) {
    return PhysicalRegion(launcher._req.region.id);
}
inline void Runtime::unmap_region(Context ctx, PhysicalRegion region) {}
inline LogicalPartition Runtime::get_logical_partition(LogicalRegion parent,
                                                       IndexPartition handle) {
    return LogicalPartition(parent);
}
inline LogicalRegion Runtime::get_logical_subregion_by_color(
    LogicalPartition parent, const DomainPoint& c) {
    return parent.region;
}
inline Future Runtime::execute_task(Context ctx,
                                    const TaskLauncher& launcher) {
    Task task(launcher._arg);
    std::vector<PhysicalRegion> regions;
    for (auto req : launcher.reqs) {
        regions.push_back(PhysicalRegion(req.region.id));
    }
    Future fut(tasks.at(launcher._tid)->run(&task, regions, ctx, this));
    futures.push_back(fut);
    return fut;
}
template <typename T,
          T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                        Context, Runtime*)>
VariantID Runtime::preregister_task_variant(
    const TaskVariantRegistrar& registrar, const char* task_name) {
    tasks.insert_or_assign(registrar.id, new RuntimeHelperT<T, TASK_PTR>);
    return registrar.id;
}
template <void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                           Context, Runtime*)>
VariantID Runtime::preregister_task_variant(
    const TaskVariantRegistrar& registrar, const char* task_name) {
    tasks.insert_or_assign(registrar.id, new RuntimeHelperT<void, TASK_PTR>);
    return registrar.id;
}
inline void* RuntimeHelper::run(const Task* task,
                                const std::vector<PhysicalRegion>& regions,
                                Context ctx, Runtime* rt) {
    return nullptr;
}
inline RuntimeHelper::~RuntimeHelper() {}
template <typename T,
          T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                        Context, Runtime*)>
void* RuntimeHelperT<T, TASK_PTR>::run(
    const Task* task, const std::vector<PhysicalRegion>& regions, Context ctx,
    Runtime* rt) {
    T val = TASK_PTR(task, regions, ctx, rt);
    void* ret = std::malloc(sizeof(T));
    memcpy(ret, &val, sizeof(T));
    return ret;
}
template <void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                           Context, Runtime*)>
void* RuntimeHelperT<void, TASK_PTR>::run(
    const Task* task, const std::vector<PhysicalRegion>& regions, Context ctx,
    Runtime* rt) {
    TASK_PTR(task, regions, ctx, rt);
    return nullptr;
}

}  // namespace Legion

#endif  // SERIAL_LEGION_INL_HH_
