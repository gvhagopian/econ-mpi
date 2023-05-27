#include <mpi.h>
#include <stdio.h>
#include <vector>

int load_data(char *filename, float *&data, int &sz) {
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

    data = (float *)malloc(data_sz_bytes);
    if (data == nullptr)
        return -1;

    rc = MPI_File_read_all(data_fh, data, data_sz_bytes, MPI_BYTE, &status);
    if (rc != MPI_SUCCESS)
        return rc;

    sz = data_sz_bytes / sizeof(float);

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

    float *data;
    int sz;

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Get_processor_name(processor_name, &name_len);

    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, rank, size);

    rc = load_data(argv[1], data, sz);

    // Finalize the MPI environment.
    MPI_Finalize();
}