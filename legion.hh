#ifndef LEGION_HH_
#define LEGION_HH_

#include <cstddef>
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
typedef long long int coord_t;
typedef ::legion_privilege_mode_t PrivilegeMode;
typedef ::legion_coherence_property_t CoherenceProperty;

/* Geometric types. */

template <unsigned int DIM>
class Point {};
template <>
class Point<1> {
public:
    Point(int p);
};
template <>
class Point<2> {
public:
    Point(int p1, int p2);
};
class DomainPoint {
public:
    template <unsigned int DIM>
    DomainPoint(const Point<DIM>& rhs);
    DomainPoint(coord_t index);
};

template <unsigned int DIM>
class Rect {
public:
    Rect(Point<DIM> lo, Point<DIM> hi);
};
class Domain {
public:
    template <unsigned int DIM>
    Domain(const Rect<DIM>& other);
};

template <unsigned int DIM>
class PointInRectIterator {
public:
    PointInRectIterator(const Rect<DIM>& r, bool column_major_order = true);
    bool operator()(void) const;
    Point<DIM> operator*(void)const;
    PointInRectIterator<DIM> operator++(int);
};

/* Memory structures. */

class IndexSpace {};
template <unsigned int DIM>
class IndexSpaceT : public IndexSpace {
public:
    IndexSpaceT(const IndexSpace& rhs);
};
class IndexPartition {};

class FieldSpace {};
class FieldAllocator {
public:
    FieldID allocate_field(size_t field_size, FieldID desired_fieldid);
};

class LogicalRegion {};
template <unsigned int DIM>
class LogicalRegionT : public LogicalRegion {
public:
    LogicalRegionT(const LogicalRegion& rhs);
};
class LogicalPartition {};

class RegionRequirement {
public:
    RegionRequirement(LogicalRegion _handle, PrivilegeMode _priv,
                      CoherenceProperty _prop, LogicalRegion _parent);
    RegionRequirement& add_field(FieldID fid);
};

class PhysicalRegion {};

template <PrivilegeMode MODE, typename FT, int N>
class FieldAccessor {
public:
    FieldAccessor(const PhysicalRegion& region, FieldID fid);
    FT& operator[](const Point<N>&) const;
};

/* Runtime types and classes. */

class Context {};

class Future {
public:
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

class Task {
public:
    void* args;
};
class TaskArgument {
public:
    TaskArgument(const void* arg, size_t argsize);
};
class TaskLauncher {
public:
    TaskLauncher(TaskID tid, TaskArgument arg);
    RegionRequirement& add_region_requirement(const RegionRequirement& req);
    void add_field(unsigned int idx, FieldID fid);
};
class TaskVariantRegistrar {
public:
    TaskVariantRegistrar(TaskID task_id, const char* variant_name);
    TaskVariantRegistrar& add_constraint(
        const ProcessorConstraint& constraint);
};

class InlineLauncher {
public:
    InlineLauncher(const RegionRequirement& req);
};

struct InputArgs {
    int argc;
    char** argv;
};

class Runtime {
public:
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
              T (*)(const Task*, const std::vector<PhysicalRegion>&, Context,
                    Runtime*)>
    static VariantID preregister_task_variant(
        const TaskVariantRegistrar& registrar, const char* task_name = NULL);
    template <void (*)(const Task*, const std::vector<PhysicalRegion>&,
                       Context, Runtime*)>
    static VariantID preregister_task_variant(
        const TaskVariantRegistrar& registrar, const char* task_name = NULL);
};
}  // namespace Legion

#endif  // LEGION_HH_
