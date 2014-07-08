#include <iostream>
#include <fstream>
#include <cstdlib>
#include <mpi.h>

using namespace std;

static const char FILENAME[] = "lattice";
static int proc_id = -1;
static int procs_count = -1;
static MPI_Status stat = { 0 };
static int rows = -1;
static int columns = -1;
static int iterations = -1;

static void ca_read_line_from_file(char *);
static int ca_count_alive(int, char *, char *, char *);
static void ca_send_buffer(char *);
static void ca_receive_buffers(char *, char *);
static void ca_print_row(char *);

int
main(int argc, char ** argv)
{
    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &procs_count);
    MPI_Comm_rank (MPI_COMM_WORLD, &proc_id);

#ifdef DEBUG
    MPI_Barrier(MPI_COMM_WORLD);
    double starttime = MPI_Wtime();
#endif

    rows = atoi(argv[1]);
    columns = atoi(argv[2]);
    iterations = atoi(argv[3]);

    char buffer_up[columns];
    char buffer[columns];
    char tmp[columns];
    char buffer_down[columns];

    // read exactly one line from the file into the buffer
    ca_read_line_from_file(buffer);

    for (int i = 0; i < iterations; ++i)
    {
        memset(buffer_up, '0', columns);
        memset(buffer_down, '0', columns);
        memset(tmp, '0', columns);
        memcpy(tmp, buffer, columns);

        if (proc_id % 2 == 0)
        {
          // send row to upper and lower neighbors
          ca_send_buffer(buffer);
          // receive row from upper and lower neighbors
          ca_receive_buffers(buffer_up, buffer_down);
        }
        else
        {
          // receive row from upper and lower neighbors
          ca_receive_buffers(buffer_up, buffer_down);
          // send row to upper and lower neighbors
          ca_send_buffer(buffer);
        }

        for (int j = 0; j < columns; ++j)
        {
            int live = ca_count_alive(j, buffer_up, tmp, buffer_down);

            // For a space that is 'populated':
            //   Each cell with one or no neighbors dies, as if by loneliness.
            //   Each cell with four or more neighbors dies, as if by overpopulation.
            //   Each cell with two or three neighbors survives.
            // For a space that is 'empty' or 'unpopulated':
            //   Each cell with three neighbors becomes populated.
            // simplified (faster?) conditions
            if (live == 3 || (live == 2 && buffer[j] == '1') ) buffer[j] = '1';
            else buffer[j] = '0';
        }
    }

    ca_print_row(buffer);

#ifdef DEBUG
    MPI_Barrier(MPI_COMM_WORLD);
    cout << proc_id << ":" << MPI_Wtime() - starttime << endl;
#endif

    MPI_Finalize ();

    return 0;
}

static void
ca_print_row(char * buffer)
{
    // print row
    printf("%d:", proc_id);
    for (int i = 0; i < columns; ++i) printf("%c", buffer[i]);
    printf("\n");
}

static void
ca_send_buffer(char * buffer)
{
    // send row to upper and lower neighbors
    if (proc_id == 0)
        MPI_Send (buffer, columns, MPI_CHAR, proc_id + 1, 0, MPI_COMM_WORLD);
    else if (proc_id == procs_count - 1)
        MPI_Send (buffer, columns, MPI_CHAR, proc_id - 1, 0, MPI_COMM_WORLD);
    else
    {
        MPI_Send (buffer, columns, MPI_CHAR, proc_id - 1, 0, MPI_COMM_WORLD);
        MPI_Send (buffer, columns, MPI_CHAR, proc_id + 1, 0, MPI_COMM_WORLD);
    }
}

static void
ca_receive_buffers(char * bup, char * bdown)
{
    // receive row from upper and lower neighbors
    if (proc_id == 0)
        MPI_Recv (bdown, columns, MPI_CHAR, proc_id + 1, 0, MPI_COMM_WORLD, &stat);
    else if (proc_id == procs_count - 1)
        MPI_Recv (bup  , columns, MPI_CHAR, proc_id - 1, 0, MPI_COMM_WORLD, &stat);
    else
    {
        MPI_Recv (bup  , columns, MPI_CHAR, proc_id - 1, 0, MPI_COMM_WORLD, &stat);
        MPI_Recv (bdown, columns, MPI_CHAR, proc_id + 1, 0, MPI_COMM_WORLD, &stat);
    }
}

static void
ca_read_line_from_file(char * buffer)
{
    ifstream file(FILENAME, ios::in | ios::binary);
    file.seekg(proc_id * (columns + 1), ios::beg);
    memset(buffer, '0', columns);
    file.read(buffer, columns);
    file.close();
}

static int
ca_count_alive(int x, char * bup, char * b, char * bdown)
{
    int live = 0;

    // nepotrebujeme kontrolovat, jestli jsme prvni nebo posledni radek
    // protoze predchozi a nasledujici radek je v kazdem cyklu automaticky nulovan
    // takze obsahuje same nuly -> nijak neovlivni "zivost" okoli

    if (x > 0)           live += bup[x-1]    == '1' ? 1 : 0;
                         live += bup[x  ]    == '1' ? 1 : 0;
    if (x < columns - 1) live += bup[x+1]    == '1' ? 1 : 0;

    if (x > 0)           live += b[x-1]      == '1' ? 1 : 0;
    if (x < columns - 1) live += b[x+1]      == '1' ? 1 : 0;

    if (x > 0)           live += bdown[x-1]  == '1' ? 1 : 0;
                         live += bdown[x  ]  == '1' ? 1 : 0;
    if (x < columns - 1) live += bdown[x+1]  == '1' ? 1 : 0;

    return live;
}
