#include "dummy_legion.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
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
bool DomainPoint::operator==(const DomainPoint& other) const {
    return coords == other.coords;
}

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
bool Domain::operator==(const Domain& other) const {
    return lo == other.lo && hi == other.hi;
}
size_t Domain::size() const {
    size_t size = 0;
    for (unsigned int i = 0; i < lo.coords.size(); i++) {
        size *= hi.coords[i] - lo.coords[i] + 1;
    }
    return size;
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

bool IndexSpace::operator==(const IndexSpace& other) const {
    return dom == other.dom;
}
size_t IndexSpace::size() const { return dom.size(); }
template <unsigned int DIM>
IndexSpaceT<DIM>::IndexSpaceT(const IndexSpace& rhs) : rect(rhs.dom) {}

bool FieldSpace::operator==(const FieldSpace& other) const {
    return fields == other.fields;
}

FieldID FieldAllocator::allocate_field(size_t field_size,
                                       FieldID desired_fieldid) {
    space.fields[desired_fieldid] = field_size;
    return desired_fieldid;
}

bool LogicalRegion::operator==(const LogicalRegion& other) const {
    return ispace == other.ispace && fspace == other.fspace;
}
template <unsigned int DIM>
LogicalRegionT<DIM>::LogicalRegionT(const LogicalRegion& rhs)
    : ispace(rhs.ispace), fspace(rhs.fspace) {}

RegionRequirement::RegionRequirement(LogicalRegion _handle,
                                     PrivilegeMode _priv,
                                     CoherenceProperty _prop,
                                     LogicalRegion _parent)
    : region(_handle) {}
RegionRequirement& RegionRequirement::add_field(FieldID fid) {
    field_ids.push_back(fid);
    return *this;
}

PhysicalRegion& PhysicalRegion::operator=(PhysicalRegion rhs) {
    lregion = rhs.lregion;
    data = rhs.data;
    return *this;
}

template <PrivilegeMode MODE, typename FT, int N>
FieldAccessor<MODE, FT, N>::FieldAccessor(const PhysicalRegion& region,
                                          FieldID fid)
    : store(region), field(fid) {}
template <PrivilegeMode MODE, typename FT, int N>
FT& FieldAccessor<MODE, FT, N>::operator[](const Point<N>& p) const {
    Domain dom = store.lregion.ispace.dom;
    size_t index = 0;
    size_t dim_prod = 1;
    for (unsigned int dim = 0; dim < p.coords.size(); dim++) {
        index += p.coords[dim] * dim_prod;
        dim_prod *= dom.hi.coords[dim] - dom.lo.coords[dim] + 1;
    }
    uint8_t* base = static_cast<uint8_t*>(store.data.at(field));
    size_t fsize = store.lregion.fspace.fields.at(field);
    return *(FT*)(base + index * fsize);
}

/* Runtime types and classes. */

Future::Future(void* _res) : res(_res) {}
template <typename T>
T Future::get_result() const {
    return *(T*)res;
}
bool Future::is_ready() const { return true; }

ProcessorConstraint::ProcessorConstraint(Processor::Kind kind) {}

TaskArgument::TaskArgument(const void* arg, size_t argsize) {
    _arg = std::malloc(argsize);
    std::memcpy(_arg, arg, argsize);
}
TaskArgument::~TaskArgument() { free(_arg); }

TaskLauncher::TaskLauncher(TaskID tid, TaskArgument arg)
    : _tid(tid), _arg(arg) {}
RegionRequirement& TaskLauncher::add_region_requirement(
    const RegionRequirement& req) {
    reqs.push_back(req);
    return reqs[reqs.size() - 1];
}
void TaskLauncher::add_field(unsigned int idx, FieldID fid) {
    reqs[idx].add_field(fid);
}

TaskVariantRegistrar::TaskVariantRegistrar(TaskID task_id,
                                           const char* variant_name)
    : id(task_id) {}
TaskVariantRegistrar& TaskVariantRegistrar::add_constraint(
    const ProcessorConstraint& constraint) {
    return *this;
}

InlineLauncher::InlineLauncher(const RegionRequirement& req) : _req(req) {}

InputArgs Runtime::input_args;
std::unordered_map<VariantID, RuntimeHelper> Runtime::tasks;

InputArgs Runtime::get_input_args() { return input_args; }
void Runtime::set_top_level_task_id(TaskID top_id) {
    top_level_task_id = top_id;
}
int Runtime::start(int argc, char** argv) {
    input_args = {.argc = argc, .argv = argv};
    Task task = {.args = nullptr};
    Runtime rt;
    tasks.at(top_level_task_id)
        .run(&task, std::vector<PhysicalRegion>(), Context(), &rt);
    return 0;
}
IndexSpace Runtime::create_index_space(Context ctx, const Domain& bounds) {
    IndexSpace is;
    is.dom = bounds;
    return is;
}
IndexPartition Runtime::create_equal_partition(Context ctx, IndexSpace parent,
                                               IndexSpace color_space) {
    return IndexPartition();
}
FieldSpace Runtime::create_field_space(Context ctx) { return FieldSpace(); }
FieldAllocator Runtime::create_field_allocator(Context ctx,
                                               FieldSpace handle) {
    FieldAllocator fa;
    fa.space = handle;
    return fa;
}
LogicalRegion Runtime::create_logical_region(Context ctx, IndexSpace index,
                                             FieldSpace fields) {
    LogicalRegion region;
    region.ispace = index;
    region.fspace = fields;
    return region;
}
PhysicalRegion Runtime::map_region(Context ctx,
                                   const InlineLauncher& launcher) {
    return materialize(launcher._req.region);
}
void Runtime::unmap_region(Context ctx, PhysicalRegion region) {}
LogicalPartition Runtime::get_logical_partition(LogicalRegion parent,
                                                IndexPartition handle) {
    LogicalPartition part;
    part.region = parent;
    return part;
}
LogicalRegion Runtime::get_logical_subregion_by_color(LogicalPartition parent,
                                                      const DomainPoint& c) {
    return parent.region;
}
Future Runtime::execute_task(Context ctx, const TaskLauncher& launcher) {
    Task task;
    task.args = launcher._arg._arg;
    std::vector<PhysicalRegion> regions;
    for (auto req : launcher.reqs) {
        regions.push_back(materialize(req.region));
    }
    void* ret = tasks.at(launcher._tid).run(&task, regions, ctx, this);
    return Future(ret);
}
PhysicalRegion Runtime::materialize(const LogicalRegion& lregion) {
    auto it = std::find(lregions.begin(), lregions.end(), lregion);
    if (it != lregions.end()) {
        return pregions.at(it - lregions.begin());
    } else {
        lregions.push_back(lregion);
        PhysicalRegion pregion;
        pregion.lregion = lregion;
        // Allocate storage space.
        for (auto fid_size : lregion.fspace.fields) {
            pregion.data[fid_size.first] =
                std::malloc(fid_size.second * lregion.ispace.size());
        }
        pregions[lregions.size() - 1] = pregion;
        return pregion;
    }
}
void* RuntimeHelper::run(const Task* task,
                         const std::vector<PhysicalRegion>& regions,
                         Context ctx, Runtime* rt) {
    return nullptr;
}

}  // namespace Legion
