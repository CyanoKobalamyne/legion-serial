#ifndef LEGION_HH_
#define LEGION_HH_

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

enum legion_privilege_mode_t {
    NO_ACCESS,
    READ_ONLY,
    REDUCE,
    READ_WRITE,
    WRITE_DISCARD,
};

enum legion_coherence_property_t {
    EXCLUSIVE,
};

namespace Legion {

typedef unsigned long FieldID;
typedef unsigned int TaskID;
typedef unsigned int VariantID;
typedef size_t FieldSpaceID;
typedef size_t RegionID;
typedef long long int coord_t;
typedef ::legion_privilege_mode_t PrivilegeMode;
typedef ::legion_coherence_property_t CoherenceProperty;

/* Geometric types. */

template <unsigned int DIM, typename T = int>
class Point {
public:
    std::vector<T> coords;

    Point(T p);
    Point(T p1, T p2);

    T& operator[](unsigned int ix);
    const T& operator[](unsigned int ix) const;
};
class DomainPoint {
public:
    std::vector<coord_t> coords;

    DomainPoint() = default;
    template <unsigned int DIM>
    DomainPoint(const Point<DIM>& rhs);
    DomainPoint(coord_t coord);
    bool operator==(const DomainPoint& other) const;
    coord_t& operator[](unsigned int ix);
    const coord_t& operator[](unsigned int ix) const;
};

template <unsigned int DIM, typename T = int>
class Rect {
public:
    Point<DIM, T> lo, hi;

    Rect(Point<DIM, T> lo_, Point<DIM, T> hi_);
};
class Domain {
public:
    DomainPoint lo, hi;

    Domain() = default;
    template <unsigned int DIM, typename T = int>
    Domain(const Rect<DIM, T>& other);
    bool operator==(const Domain& other) const;
    size_t size() const;
};

template <unsigned int DIM, typename T = int>
class PointInRectIterator {
public:
    Point<DIM, T> start, cur, end;
    bool col_major;

    PointInRectIterator(const Rect<DIM, T>& r, bool column_major_order = true);
    bool operator()(void) const;
    Point<DIM, T> operator*(void) const;
    PointInRectIterator<DIM, T> operator++(int);
};

/* Memory structures. */

class IndexSpace {
public:
    Domain dom;

    IndexSpace(Domain _dom);
    bool operator==(const IndexSpace& other) const;
    size_t size() const;
};
template <unsigned int DIM>
class IndexSpaceT : public IndexSpace {
public:
    IndexSpaceT(const IndexSpace& rhs);
};
class IndexPartition {};

class FieldSpace {
public:
    FieldSpaceID id;

    FieldSpace(FieldSpaceID fsid);
    bool operator==(const FieldSpace& other) const;
};
class FieldAllocator {
public:
    FieldSpaceID id;

    FieldAllocator(FieldSpaceID _id);
    FieldID allocate_field(size_t field_size, FieldID desired_fieldid);
};

class LogicalRegion {
public:
    RegionID id;

    LogicalRegion(RegionID _id);
    bool operator==(const LogicalRegion& other) const;
};
template <unsigned int DIM>
class LogicalRegionT : public LogicalRegion {
public:
    LogicalRegionT(const LogicalRegion& rhs);
};
class LogicalPartition {
public:
    LogicalRegion region;

    LogicalPartition(LogicalRegion _region);
};

class RegionRequirement {
public:
    LogicalRegion region;
    std::vector<FieldID> field_ids;

    RegionRequirement(LogicalRegion _handle, PrivilegeMode _priv,
                      CoherenceProperty _prop, LogicalRegion _parent);
    RegionRequirement& add_field(FieldID fid);
};

class PhysicalRegion {
public:
    RegionID id;

    PhysicalRegion(RegionID _id);
    size_t get_index(const DomainPoint& p) const;
};

template <PrivilegeMode MODE, typename FT, int N>
class FieldAccessor {
public:
    PhysicalRegion store;
    FieldID field;

    FieldAccessor(const PhysicalRegion& region, FieldID fid);
    FT& operator[](const Point<N>& p) const;
};

/* Runtime types and classes. */

class Context {};

class Future {
public:
    void* res = nullptr;

    Future(void* _res);
    template <typename T>
    T get_result() const;
    bool is_ready() const;
};

class Processor {
public:
    enum Kind {
        NO_KIND,
        LOC_PROC,
    };
};
class ProcessorConstraint {
public:
    ProcessorConstraint(Processor::Kind kind = Processor::NO_KIND);
};

class TaskArgument {
public:
    const void* _arg;
    size_t _argsize;

    TaskArgument(const void* arg, size_t argsize);
};
class Task {
public:
    void* args;

    Task(TaskArgument ta);
    ~Task();
};
class TaskLauncher {
public:
    TaskID _tid;
    TaskArgument _arg;
    std::vector<RegionRequirement> reqs;

    TaskLauncher(TaskID tid, TaskArgument arg);
    RegionRequirement& add_region_requirement(const RegionRequirement& req);
    void add_field(unsigned int idx, FieldID fid);
};
class TaskVariantRegistrar {
public:
    TaskID id;

    TaskVariantRegistrar(TaskID task_id, const char* variant_name);
    TaskVariantRegistrar& add_constraint(
        const ProcessorConstraint& constraint);
};

class InlineLauncher {
public:
    RegionRequirement _req;

    InlineLauncher(const RegionRequirement& req);
};

struct InputArgs {
    int argc;
    char** argv;
};

class RuntimeHelper;
class Runtime {
public:
    inline static InputArgs input_args;
    inline static TaskID top_level_task_id;
    inline static std::unordered_map<VariantID, RuntimeHelper*> tasks;
    inline static std::vector<std::unordered_map<FieldID, size_t>>
        field_spaces;
    inline static std::vector<std::pair<IndexSpace, FieldSpace>>
        logical_regions;
    inline static std::vector<std::unordered_map<FieldID, void*>>
        physical_regions;
    inline static std::vector<Future> futures;

    static InputArgs get_input_args();
    static void set_top_level_task_id(TaskID top_id);
    static int start(int argc, char** argv);
    IndexSpace create_index_space(Context ctx, const Domain& bounds);
    IndexPartition create_equal_partition(Context ctx, IndexSpace parent,
                                          IndexSpace color_space);
    FieldSpace create_field_space(Context ctx);
    FieldAllocator create_field_allocator(Context ctx, FieldSpace handle);
    LogicalRegion create_logical_region(Context ctx, IndexSpace index,
                                        FieldSpace fields);
    PhysicalRegion map_region(Context ctx, const InlineLauncher& launcher);
    void unmap_region(Context ctx, PhysicalRegion region);
    LogicalPartition get_logical_partition(LogicalRegion parent,
                                           IndexPartition handle);
    LogicalRegion get_logical_subregion_by_color(LogicalPartition parent,
                                                 const DomainPoint& c);
    Future execute_task(Context ctx, const TaskLauncher& launcher);
    template <typename T,
              T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                            Context, Runtime*)>
    static VariantID preregister_task_variant(
        const TaskVariantRegistrar& registrar, const char* task_name = NULL);
    template <void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                               Context, Runtime*)>
    static VariantID preregister_task_variant(
        const TaskVariantRegistrar& registrar, const char* task_name = NULL);
};
class RuntimeHelper {
public:
    virtual void* run(const Task* task,
                      const std::vector<PhysicalRegion>& regions, Context ctx,
                      Runtime* rt);
    virtual ~RuntimeHelper();
};
template <typename T,
          T (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                        Context, Runtime*)>
class RuntimeHelperT : public RuntimeHelper {
public:
    void* run(const Task* task, const std::vector<PhysicalRegion>& regions,
              Context ctx, Runtime* rt);
};
template <void (*TASK_PTR)(const Task*, const std::vector<PhysicalRegion>&,
                           Context, Runtime*)>
class RuntimeHelperT<void, TASK_PTR> : public RuntimeHelper {
public:
    void* run(const Task* task, const std::vector<PhysicalRegion>& regions,
              Context ctx, Runtime* rt);
};
}  // namespace Legion

#include "dummy_legion.ii"

#endif  // LEGION_HH_
