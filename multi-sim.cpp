#include <mpi.h>
#include <stdio.h>
#include <vector>
#include <ceres/ceres.h>
#include <ceres/autodiff_first_order_function.h>

struct LeastSquares {

    LeastSquares(double *data, int sz, int proc, int nproc) :
        data_(data), sz_(sz), proc_(proc), nproc_(nproc) {
        start = (sz / nproc) * proc;
        end = start + (sz / nproc) > sz ? sz : start + (sz / nproc);
    }

    template <typename T>
    bool operator()(const T* parameters, T* cost) const {
        const T x = parameters[0];
        int i;

        T sum = T(0.0);

        for (i = start; i < end; ++i) {
            sum += data_[i] * data_[i] * x * x;
        }

        MPI_Reduce(&sum, &cost[0], 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        return true;
    }

    static ceres::FirstOrderFunction* Create(double *data, int sz, int proc, int nproc) {
        constexpr int kNumParameters = 1;
        return new ceres::AutoDiffFirstOrderFunction<LeastSquares, kNumParameters>(
            new LeastSquares(data, sz, proc, nproc));
    }

    double *data_;
    int sz_;

    int proc_;
    int nproc_;

    int start;
    int end;
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
    int proc, nproc, rc;

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

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc);

    MPI_Get_processor_name(processor_name, &name_len);

    rc = load_data(argv[1], data, sz);
    if (rc != MPI_SUCCESS) {
        MPI_Finalize();
        exit(rc);
    }

    double parameters[1] = {-1.2};

    ceres::GradientProblem problem(LeastSquares::Create(data, sz, proc, nproc));

    ceres::GradientProblemSolver::Options options;
    ceres::GradientProblemSolver::Summary summary;
    ceres::Solve(options, problem, parameters, &summary);

    std::cout << summary.FullReport() << std::endl;

    // Finalize the MPI environment.
    MPI_Finalize();
}