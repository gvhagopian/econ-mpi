#include <mpi.h>
#include <stdio.h>
#include <vector>
#include <ceres/ceres.h>

struct LeastSquares {

    LeastSquares(double *data, int sz) : data_(data), sz_(sz) {}

    template <typename T>
    bool operator()(const T* parameters, T* cost) const {
        const T x = parameters[0];

        cost[0] = cost[0] * 0.0;
        for (int i = 0; i < sz_; ++i) {
            cost[0] += data_[i] * x * data_[i] * x;
        }

        return true;
    }

    static ceres::CostFunction* Create(double *data, int sz) {
        constexpr int kNumParameters = 1;
        return new ceres::AutoDiffCostFunction<LeastSquares, kNumParameters, 1>(
            new LeastSquares(data, sz));
    }

    double *data_;
    int sz_;
};

int load_data(char *filename, double *&data, int &sz) {
    int rc;
    MPI_Offset data_sz_bytes;

    MPI_File data_fh;
    MPI_Status status;

    rc = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL,
        &data_fh);
    if (rc != MPI_SUCCESS)
        return rc;

    rc = MPI_File_get_size(data_fh, &data_sz_bytes);
    if (rc != MPI_SUCCESS)
        return rc;

    data = (double *)malloc(data_sz_bytes);
    if (data == nullptr)
        return -1;

    printf("%p\n", data);
    fflush(stdout);

    rc = MPI_File_read_all(data_fh, data, data_sz_bytes, MPI_BYTE, &status);
    if (rc != MPI_SUCCESS)
        return rc;

    sz = data_sz_bytes / sizeof(double);

    rc = MPI_File_close(&data_fh);
    if (rc != MPI_SUCCESS)
        return rc;

    return 0;
}

int main(int argc, char **argv) {
    int size, rank, rc;
    int data_size;

    int name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    double *data;
    int sz;

    if (argc <= 1) {
        std::cout << "Usage: " << argv[0] << " [filename]" << std::endl;
        return 1;
    }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Get_processor_name(processor_name, &name_len);

    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, rank, size);

    rc = load_data(argv[1], data, sz);
    if (rc != MPI_SUCCESS) {
        MPI_Finalize();
        exit(rc);
    }

    double initial_x = 5.0;
    double x = initial_x;

    ceres::Problem problem;

    ceres::CostFunction *cost_function = LeastSquares::Create(data, sz);

    problem.AddResidualBlock(cost_function, nullptr, &x);

    ceres::Solver::Options options;
    options.linear_solver_type = ceres::DENSE_QR;
    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);

    std::cout << summary.BriefReport() << std::endl;
    // Finalize the MPI environment.
    MPI_Finalize();
}