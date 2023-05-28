#include <stdio.h>
#include <vector>
#include <mpi.h>
#include <omp.h>
#include <ceres/ceres.h>
#include <ceres/autodiff_first_order_function.h>

struct LeastSquares {

    LeastSquares(std::vector<double> &data_x, std::vector<double> &data_y,
        std::vector<double> &data_z, int proc, int nproc) :
        data_x_(data_x), data_y_(data_y),  data_z_(data_z),
        proc_(proc), nproc_(nproc)
    {
        sz = data_x.size();
        start = (sz / nproc) * proc;
        end = start + (sz / nproc) > sz ? sz : start + (sz / nproc);
    }

    template <typename T>
    bool operator()(const T* parameters, T* cost) const {
        const T x = parameters[0];
        const T z = parameters[1];
        int i;

        // accumulate per-slot sums and per-thread sums separately
        std::vector<T> partial_sums_mpi(nproc_, T(0.0));
        std::vector<T> partial_sums_omp(omp_get_num_procs(), T(0.0));

        T sum = T(0.0);
        T diff;

        // start and end already define a subset of the total computation;
        // further divide the task between available threads with OMP
        #pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            #pragma omp for private(diff)
            for (i = start; i < end; ++i) {
                diff = data_y_[i] - x * data_x_[i] - z * data_z_[i];
                partial_sums_omp[thread_id] += diff * diff;
            }
        }

        sum = std::reduce(partial_sums_omp.begin(), partial_sums_omp.end());

        MPI_Allgather(&sum, sizeof(T), MPI_CHAR, &partial_sums_mpi[0], sizeof(T),
            MPI_CHAR, MPI_COMM_WORLD);

        cost[0] = std::reduce(partial_sums_mpi.begin(), partial_sums_mpi.end());

        return true;
    }

    static ceres::FirstOrderFunction* Create(std::vector<double> &data_x,
        std::vector<double> &data_y, std::vector<double> &data_z, int proc, int nproc) {
        constexpr int kNumParameters = 2;
        return new ceres::AutoDiffFirstOrderFunction<LeastSquares, kNumParameters>(
            new LeastSquares(data_x, data_y, data_z, proc, nproc));
    }

    size_t sz;

    std::vector<double> &data_x_;
    std::vector<double> &data_y_;
    std::vector<double> &data_z_;

    int proc_;
    int nproc_;

    int start;
    int end;
};

int load_data(char *filename, std::vector<double> &data) {
    int rc;
    size_t sz;
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

    double *buf = (double *)malloc(data_sz_bytes);
    if (buf == nullptr)
        return -1;

    rc = MPI_File_read_all(data_fh, buf, data_sz_bytes, MPI_BYTE, &status);
    if (rc != MPI_SUCCESS)
        return rc;

    sz = data_sz_bytes / sizeof(double);

    rc = MPI_File_close(&data_fh);
    if (rc != MPI_SUCCESS)
        return rc;

    data = std::vector<double>(buf, buf + sz);

    return 0;
}

int main(int argc, char **argv) {
    int proc, nproc, rc;

    int name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    std::vector<double> data_x;
    std::vector<double> data_y;
    std::vector<double> data_z;

    if (argc <= 2) {
        std::cout << "Usage: " << argv[0] << " [filename_x] [filename_y] [filename_z]" <<
            std::endl;
        return 1;
    }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc);

    MPI_Get_processor_name(processor_name, &name_len);

    if ((rc = load_data(argv[1], data_x)) != MPI_SUCCESS ||
            (rc = load_data(argv[2], data_y)) != MPI_SUCCESS ||
            (rc = load_data(argv[3], data_z)) != MPI_SUCCESS) {
        MPI_Finalize();
        exit(rc);
    }

    double parameters[2] = {1.0, 1.0};

    ceres::GradientProblem problem(LeastSquares::Create(data_x, data_y, data_z,
        proc, nproc));

    ceres::GradientProblemSolver::Options options;
    ceres::GradientProblemSolver::Summary summary;
    ceres::Solve(options, problem, parameters, &summary);

    if (proc == 0)
        std::cout << parameters[0] << ", " << parameters[1] << std::endl
            << summary.FullReport() << std::endl;

    // Finalize the MPI environment.
    MPI_Finalize();
}