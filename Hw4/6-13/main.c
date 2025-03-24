/* Author by 姜任懋, 蓋彥文, 邱亮茗 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <mpi.h>
#include <string.h>
#include <sys/syscall.h>

#define PRINT_MATRIX_IN_SEQ()                        			   	\
    for (int i = 0; i < row_num; i++) {         	  			 	\
        for (int j = 0; j < col_num; j++) {    	  				 	\
            printf("%d ", buf[LINEAR_ADDRESS(i, j, col_num)]); 		\
        }                                    				        \
        printf("\n");                              					\
    }

#define BLOCK_LOW(id, p, n)  ((id) * (n) / (p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id) + 1, p, n) - 1)
#define BLOCK_NUMS(id, p, n) (BLOCK_LOW((id) + 1, p, n) - BLOCK_LOW(id, p, n)) 
#define LINEAR_ADDRESS(i, j, col) ((i) * (col) + (j))
#define DOWNSTREAM_MSG		1
#define UPSTREAM_MSG		2
#define DUMMY_MSG		3
#define OUTPUT_MSG		4

int output_num;
char output_buf[65536];

int proc_num;
int proc_idx;
int row_num;
int col_num;

int blk_num;
int cell_num;
int file_offset;


void show_sub_matrix (bool* sub_matrix) {
	for (int i = 0; i < blk_num; i++) {
		for(int j = 0; j < col_num; j++) {
			printf("%d ", sub_matrix[i * col_num + j]);
		}
		printf("\n");
	}
	fflush(stdout);
	
	/*
	for (int i = 0; i < blk_num; i++) {
		output_num = 0;
		for (int j = 0; j < col_num; j++) {
			if (sub_matrix[i * col_num + j]) {
				sprintf(output_buf + output_num, "%c", '1');
			} else {
				sprintf(output_buf + output_num, "%c", '0');
			}
			output_num++;
			sprintf(output_buf + output_num, "%c", ' ');
			output_num++;
		}
		sprintf(output_buf + output_num, "%c", '\n');
		output_num++;
		//write(1, output_buf, output_num);
		syscall(SYS_write, 1, output_buf, output_num);

	}
	*/
}

void update_row (bool* top_row, bool* middle_row, bool* bottom_row, bool* updated_sub_matrix, int updated_row_idx)
{
	for (int idx = 0; idx < col_num; idx++){
		char life_cnt = 0;
		/*	top row	*/
		if (top_row) {
			if (idx > 0) life_cnt += top_row[idx - 1];
			life_cnt += top_row[idx];
			if (idx < col_num - 1) life_cnt += top_row[idx + 1];
		}

		/*	middle row	*/
		if (idx > 0) life_cnt += middle_row[idx - 1];
		if (idx < col_num - 1) life_cnt += middle_row[idx + 1];

		/*	bottom row	*/
		if (bottom_row) {
			if (idx > 0) life_cnt += bottom_row[idx - 1];
			life_cnt += bottom_row[idx];
			if (idx < col_num - 1) life_cnt += bottom_row[idx + 1];
		}
		
		/*	debugging	*/
		//printf("Updating row %d col %d of process %d.........., life number = %d\n", updated_row_idx, idx, proc_idx, life_cnt);
		/*	debugging	*/

		if (middle_row[idx]) {
			if (life_cnt != 2 && life_cnt != 3)	updated_sub_matrix[updated_row_idx * col_num + idx] = false;
			else updated_sub_matrix[updated_row_idx * col_num + idx] = true; 
		} else {
			if (life_cnt == 3) updated_sub_matrix[updated_row_idx * col_num + idx] = true;
			else updated_sub_matrix[updated_row_idx * col_num + idx] = false;
		}
	}
}


void update_sub_matrix (bool** sub_matrix) 
{
	int first_row = 0;
	int last_row = blk_num - 1;

	bool* first_row_recv_buf;
	bool* last_row_recv_buf;

	first_row_recv_buf = malloc(sizeof(bool) * col_num);
	if (first_row_recv_buf == NULL) {
		fprintf(stderr, "fail to allocate receive first row buffer memory from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	last_row_recv_buf = malloc(sizeof(bool) * col_num);
	if (last_row_recv_buf == NULL) {
		fprintf(stderr, "fail to allocate receive last row buffer memory from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	bool* updated_sub_matrix;
	updated_sub_matrix = malloc(sizeof(bool) * cell_num);
	if (updated_sub_matrix == NULL) {
		fprintf(stderr, "fail to allocate updated sub-matrix memory from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	if (proc_idx == 0) {
		MPI_Send(*sub_matrix + last_row * col_num, col_num, MPI_CHAR, proc_idx + 1, DOWNSTREAM_MSG, MPI_COMM_WORLD);
		MPI_Recv(last_row_recv_buf, col_num, MPI_CHAR, proc_idx + 1, UPSTREAM_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		free(first_row_recv_buf);
		first_row_recv_buf = NULL;
	} else if (proc_idx == proc_num - 1) {
		MPI_Send(*sub_matrix + first_row * col_num, col_num, MPI_CHAR, proc_idx - 1, UPSTREAM_MSG, MPI_COMM_WORLD);
		MPI_Recv(first_row_recv_buf, col_num, MPI_CHAR, proc_idx - 1, DOWNSTREAM_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		free(last_row_recv_buf);
		last_row_recv_buf = NULL;
	} else {
		MPI_Send(*sub_matrix + last_row * col_num, col_num, MPI_CHAR, proc_idx + 1, DOWNSTREAM_MSG, MPI_COMM_WORLD);
		MPI_Send(*sub_matrix + first_row * col_num, col_num, MPI_CHAR, proc_idx - 1, UPSTREAM_MSG, MPI_COMM_WORLD);
		MPI_Recv(first_row_recv_buf, col_num, MPI_CHAR, proc_idx - 1, DOWNSTREAM_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(last_row_recv_buf, col_num, MPI_CHAR, proc_idx + 1, UPSTREAM_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	/*	debugging	*/
	/*
	if (first_row_recv_buf) {
		printf("first row of process %d :\n", proc_idx);
		for (int i = 0; i < col_num; i++) {
			printf("%d ", first_row_recv_buf[i]);
		}
		printf("\n");
	} else {
		printf("first row of process %d : NULL\n", proc_idx);
	}
	
	if (last_row_recv_buf) {
		printf("last row of process %d :\n", proc_idx);
		for (int i = 0; i < col_num; i++) {
			printf("%d ", last_row_recv_buf[i]);
		}
		printf("\n");
	} else {
		printf("last row of process %d : NULL\n", proc_idx);
	}
	*/
	/*	debugging	*/


	for (int i = first_row; i <= last_row; i++) {
		if (i == first_row && i == last_row) {
			update_row(first_row_recv_buf, *sub_matrix + i * col_num, last_row_recv_buf, updated_sub_matrix, i);
		} else if (i == first_row && i != last_row) {
			update_row(first_row_recv_buf, *sub_matrix + i * col_num, *sub_matrix + (i + 1) * col_num, updated_sub_matrix, i);
		} else if (i != first_row && i == last_row) {
			update_row(*sub_matrix + (i - 1) * col_num, *sub_matrix + i * col_num, last_row_recv_buf, updated_sub_matrix, i);
		} else {
			update_row(*sub_matrix + (i - 1) * col_num, *sub_matrix + i * col_num, *sub_matrix + (i + 1) * col_num, updated_sub_matrix, i);
		}
	}
	free(*sub_matrix);
	*sub_matrix = updated_sub_matrix;
	if (first_row_recv_buf) free(first_row_recv_buf);
	if (last_row_recv_buf) free(last_row_recv_buf);
}

void periolically_show (bool** sub_matrix, int iteration, int freq) {
	char dummy;
	int cnt = 0;
	bool* recv_buf;
	bool* out_ptr;
	if (!proc_idx) {
		recv_buf = malloc(sizeof(bool) * BLOCK_NUMS(proc_num - 1, proc_num, row_num) * col_num);
	}
	for (int x = -1; x < iteration; x++) {
		if (x == -1 || ++cnt == freq) {
			if (proc_idx == 0) {
				//printf("iteration %d :\n", x + 1);
				//sprintf(output_buf, "iteration %d :\n", x + 1);
				//write(1, output_buf, strlen(output_buf));
				//syscall(SYS_write, 1, output_buf, strlen(output_buf));
				//show_sub_matrix(*sub_matrix);
				printf("iteration %d :\n", x + 1);
				MPI_Send(&dummy, 1, MPI_CHAR, proc_idx + 1, DUMMY_MSG, MPI_COMM_WORLD);
				for (int i = 0; i < proc_num; i++) {
					if (i != 0) {
						out_ptr = recv_buf;
						MPI_Recv(recv_buf, BLOCK_NUMS(i, proc_num, row_num) * col_num, MPI_CHAR, i, OUTPUT_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					} else {
						out_ptr = *sub_matrix;
					}
					for (int row_idx = 0; row_idx < BLOCK_NUMS(i, proc_num, row_num); row_idx++) {
						for (int col_idx = 0; col_idx < col_num; col_idx++) {
							printf("%d ", *(out_ptr + row_idx * col_num + col_idx));
						}
						printf("\n");
					}
				}
			} else if (proc_idx == proc_num - 1) {
				MPI_Recv(&dummy, 1, MPI_CHAR, proc_idx - 1, DUMMY_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Send(*sub_matrix, cell_num, MPI_CHAR, 0, OUTPUT_MSG, MPI_COMM_WORLD);
			} else {
				MPI_Recv(&dummy, 1, MPI_CHAR, proc_idx - 1, DUMMY_MSG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Send(*sub_matrix, cell_num, MPI_CHAR, 0, OUTPUT_MSG, MPI_COMM_WORLD);
				MPI_Send(&dummy, 1, MPI_CHAR, proc_idx + 1, DUMMY_MSG, MPI_COMM_WORLD);	
			}
			cnt = 0;
		}
		if (x != iteration - 1) update_sub_matrix(sub_matrix);
	}
	if (!proc_idx) free(recv_buf);
}

void seq_sol (int fd, int iter, int freq) {
	ssize_t read_num;
	double start_time, end_time;

	/*	read row nums	*/
	if ((read_num = read(fd, &row_num, sizeof(int))) == -1) {
		fprintf(stderr, "Read row error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	/*	read column nums	*/
	if ((read_num = read(fd, &col_num, sizeof(int))) == -1) {
		fprintf(stderr, "Read column error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	
	bool* buf;
	bool* tmp;
	buf = malloc(sizeof(bool) * row_num * col_num);
	if (buf == NULL) {
		fprintf(stderr, "fail to allocate memory from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	tmp = malloc(sizeof(bool) * row_num * col_num);
	if (tmp == NULL) {
		fprintf(stderr, "fail to allocate memory from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	if ((read_num = read(fd, buf, row_num * col_num * sizeof(bool))) == -1 || read_num != row_num * col_num * sizeof(bool)) {
		fprintf(stderr, "Read matrix error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	
	start_time = MPI_Wtime();
	int times = 0;
	for (int k = 0; k < iter; k++) {
		if (k == 0) {
			//printf("initial state :\n");
			//PRINT_MATRIX_IN_SEQ();
		}

		for (int i = 0; i < row_num; i++) {
			for (int j = 0; j < col_num; j++) {
				int cnt = 0;
				if (i > 0) {
					if (j > 0) cnt += buf[LINEAR_ADDRESS(i - 1, j - 1, col_num)];
					cnt += buf[LINEAR_ADDRESS(i - 1, j, col_num)];
					if (j < col_num - 1) cnt += buf[LINEAR_ADDRESS(i - 1, j + 1, col_num)];
				}

				if (j > 0) cnt += buf[LINEAR_ADDRESS(i, j - 1, col_num)];
				if (j < col_num - 1) cnt += buf[LINEAR_ADDRESS(i, j + 1, col_num)];

				if (i < row_num - 1) {
					if (j > 0) cnt += buf[LINEAR_ADDRESS(i + 1, j - 1, col_num)];
					cnt += buf[LINEAR_ADDRESS(i + 1, j, col_num)];
					if (j < col_num - 1) cnt += buf[LINEAR_ADDRESS(i + 1, j + 1, col_num)];
				}

				if(buf[LINEAR_ADDRESS(i, j, col_num)]) {
					if (cnt != 2 && cnt != 3) tmp[LINEAR_ADDRESS(i, j, col_num)] = false;
					else tmp[LINEAR_ADDRESS(i, j, col_num)] = true;
				} else {
					if (cnt == 3) tmp[LINEAR_ADDRESS(i, j, col_num)] = true;
					else tmp[LINEAR_ADDRESS(i, j, col_num)] = false;
				}
				//printf("(%d, %d) : %d\n", i, j, cnt);
			}
		}

		tmp = (bool*)((long)tmp ^ (long)buf);
		buf = (bool*)((long)tmp ^ (long)buf);
		tmp = (bool*)((long)tmp ^ (long)buf);

		if (++times == freq) {
			//printf("iteration %d :\n", k + 1);
			//PRINT_MATRIX_IN_SEQ();
			times = 0;
		}
	}

	end_time = MPI_Wtime();
	printf("real execution time: %.10f seconds\n", end_time - start_time);
	
	free(buf);
	free(tmp);
}

int main (int argc, char** argv) {
	int fd;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_idx);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_num);

	if (!proc_idx && argc != 4) {
		fprintf(stderr, "Usage: %s [file name] [iteration] [frequency]\n", argv[0]);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	char* file_name = argv[1];
	int iteration = atoi(argv[2]);
	int display_freq = atoi(argv[3]);

	MPI_Barrier(MPI_COMM_WORLD);

	/*	open the matrix file	*/
	fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Fail to open file : matrix.bin from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	if (proc_num == 1) {
		seq_sol(fd, iteration, display_freq);
		MPI_Finalize();
		exit(0);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	
	ssize_t read_num;
	/*	read row nums	*/
	if ((read_num = read(fd, &row_num, sizeof(int))) == -1) {
		fprintf(stderr, "Read row error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	if (row_num < proc_num && !proc_idx) {
		fprintf(stderr, "Row size < process num\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	/*	read column nums	*/
	if ((read_num = read(fd, &col_num, sizeof(int))) == -1) {
		fprintf(stderr, "Read column error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	/*	Calculate the local block num and local cell num	*/
	/*	and allocate the sub-matrix for cells. Then read	*/
	/*	the cells from matrix.bin.				*/
	bool* sub_matrix;
	blk_num = BLOCK_NUMS(proc_idx, proc_num, row_num);
	cell_num = blk_num * col_num;
	sub_matrix = malloc(sizeof(bool) * cell_num);
	if (sub_matrix == NULL) {
		fprintf(stderr, "Fail to allocate sub-matrix memory space from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	file_offset = BLOCK_LOW(proc_idx, proc_num, row_num) * col_num * sizeof(bool);
	lseek(fd, file_offset, SEEK_CUR);
	if ((read_num = read(fd, sub_matrix, cell_num * sizeof(bool))) == -1 || read_num != cell_num * sizeof(bool)) {
		fprintf(stderr, "Read sub-matrix error from process %d\n", proc_idx);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	

	/*	Show the sub-matrix for debugging	*/
	/*
	printf("Sub-matrix from process %d : \n", proc_idx);
	for (int i = 0; i < blk_num; i++) {
		for (int j = 0; j < col_num; j++) {
			printf("%d ", sub_matrix[i * col_num + j]);
		}
		printf("\n");
	}
	printf("\n");
	*/

	double start_time, end_time;
	double real_start_time, real_end_time;

	MPI_Barrier(MPI_COMM_WORLD);
	start_time = MPI_Wtime();
	periolically_show(&sub_matrix, iteration, display_freq);
	end_time = MPI_Wtime();
	MPI_Reduce(&start_time, &real_start_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&end_time, &real_end_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (!proc_idx) {
		FILE* fp;
		fp = fopen("./out", "w");
		fprintf(fp, "real execution time: %lf seconds\n", real_end_time - real_start_time);
		fprintf(fp, "start time : %.10f, end time : %.10f, real start time : %.10f, real end time : %.10f\n", start_time, end_time, real_start_time, real_end_time);
	}
	
	free(sub_matrix);
	MPI_Finalize();
	exit(0);
}
