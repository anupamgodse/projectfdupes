#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "7.h"
int main(int argc, char **argv) {
	struct timeval stop, start;
	myplan *mp = (myplan *)malloc(sizeof(myplan));
	if(mp == NULL) {
		printf("Out of Memory :\n");
		exit(1);
	}
	mp->head = mp->tail = NULL;
	mp->count = 0;
	if(argc == 1) {
		perror("No directory specified");
		exit(1);
	}
	char *myname = (char *)malloc(strlen(argv[1]) + 1);
	if(myname == NULL) {
		printf("Out of Memory :\n");
		exit(1);
	}
	strcpy(myname, argv[1]);
	gettimeofday(&start, NULL);
	sortdir(mp, myname);
	gettimeofday(&stop, NULL);
	printf("1time = %lu\n", stop.tv_usec - start.tv_usec);
	gettimeofday(&start, NULL);
	linkbysize(mp);
	gettimeofday(&stop, NULL);
	printf("2time = %lu\n", stop.tv_usec - start.tv_usec);
	gettimeofday(&start, NULL);
	linkbyhashing(mp);
	gettimeofday(&stop, NULL);
	printf("3time = %lu\n", stop.tv_usec - start.tv_usec);
	mydir *p = mp->head;
	node *q;
	while(p) {
		q = p->nhead;
		while(q) {
			q->dup = NULL;
			q = q->nnext;
		}
		p = p->mnext;
	}
	gettimeofday(&start, NULL);
	finallink(mp);
	gettimeofday(&stop, NULL);
	printf("4time = %lu\n", stop.tv_usec - start.tv_usec);
	gettimeofday(&start, NULL);
	printmyplan(mp);
	gettimeofday(&stop, NULL);
	printf("5time = %lu\n", stop.tv_usec - start.tv_usec);
	return 0;
}
void sortdir(myplan *mp, char *myname) {
	//struct timeval stop, start;
	//gettimeofday(&start, NULL);
	dirent *dir;
	DIR *dp;
	dp = opendir(myname);
	if (!dp) {
		perror("Open Failed\n");
		exit(errno);
	}
	mydir *ndir = (mydir *)malloc(sizeof(mydir));
	if(ndir == NULL) {
		printf("Out of Memory :\n");
		exit(1);
	}
	ndir->nhead = ndir->ntail = NULL;
	ndir->myname = (char *)malloc(strlen(myname) + 2);
	if(ndir->myname == NULL) {
		printf("Out of Memory :\n");
		exit(1);
	}
	strcpy(ndir->myname, myname);
	strcat(ndir->myname, "/");
	free(myname);
	appendmyplan(mp, ndir);
	while((dir = readdir(dp))) {
		if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
			if(dir->d_type == DT_REG) {
				node *mn = (node *)malloc(sizeof(node));
				if(mn == NULL) {
					printf("Out of Memory :\n");
					exit(1);
				}
				mn->nnext = mn->nprev = mn->dup = mn->dupsign = NULL;
				mn->visited = 0;
				mn->mypath = (char *)malloc(strlen(dir->d_name) + strlen(ndir->myname) + 1);
				if(mn->mypath == NULL) {
					printf("Out of Memory :\n");
					exit(1);
				}
				strcpy(mn->mypath, ndir->myname);
				strcat(mn->mypath, dir->d_name);
				(mp->count)++;
				appendmydir(ndir, mn);
			}
			else if(dir->d_type == DT_DIR) {
				char *myname = (char *)malloc(strlen(ndir->myname) + strlen(dir->d_name) + 1);
				if(myname == NULL) {
					printf("Out of Memory :\n");
					exit(1);
				}
				strcpy(myname, ndir->myname);
				strcat(myname, dir->d_name);
				sortdir(mp, myname);
			}
		}
	}
	closedir(dp);
	//gettimeofday(&stop, NULL);
	//printf("time = %lu\n", stop.tv_usec - start.tv_usec);
}
void appendmydir(mydir *md, node *mn) {
	mn->nnext = NULL;
	if(!(md->nhead)) 
		md->nhead = mn;
	else 
		md->ntail->nnext = mn;
	mn->nprev = md->ntail;
	md->ntail = mn;
}
void appendmyplan(myplan *mp, mydir *md) {
	md->mnext = NULL;
	if(!(mp->head))
		mp->head = md;
	else
		mp->tail->mnext = md;
	md->mprev = mp->tail;
	mp->tail = md;
}
void printmyplan(myplan *mp) {
	mydir *p = mp->head;
	node *q, *n;
	while(p) {
		q = p->nhead;
		while(q) {
			if(q->visited == 0 && q->dup) {
				n = q;
				while(n) {
					printf("%s\n", n->mypath);
					n = n->dup;
				}
				printf("\n");
			}
			q = q->nnext; 
		}
		p = p->mnext;
	}
}
void linkbysize(myplan *mp) {
	mydir *p = mp->head;
	node *q;
	int i = 0, k;
	flink arr[mp->count];
	while(p) {
		q = p->nhead;
		while(q) {
			q->fsize = filesize(q->mypath);
			for(k = 0; k < i; k++) {
				if(arr[k].size == q->fsize) 
					break;	
			}
			if(k == i) {
				arr[i].tail = q;
				arr[i].size = q->fsize;
				i++;
			}
			else {
				arr[k].tail->dup = q;
				arr[k].tail = q;
			}
			q = q->nnext;
		}
		p = p->mnext;	
	} 
	
}
void linkbyhashing(myplan *mp) {
	char *s[mp->count], *s1, *s2;
	mydir *p = mp->head;
	node *q, *n, *r;
	int i = 0, k, state;
	while(p) {
		q = p->nhead;	
		while(q) {
			state = 0;
			n = r = q;
			s1 = getsign(q->mypath);
			for(k = 0; k < i; k++) {  
				if(s[k] == s1) {
					q = q->nnext;
					state = 1;
					break;
				}
			}
			if(state == 1)
				continue;
			if(k == i && q->dup)
				s[i++] = s1;
			while(n->dup) {
				s2 = getsign(n->dup->mypath);
				if(strcmp(s1, s2) == 0) { 
					r->dupsign = n->dup;
					r = n->dup;
				}
				n = n->dup;
				free(s2);
			}
			q = q->nnext;
			//free(s1);
		}
		p = p->mnext; 
	}
}
void finallink(myplan *mp) {
	mydir *p = mp->head;
	struct timeval stop, start;
	node *q, *n, *r;
	int result, count = 0;
	while(p) {
		q = p->nhead;	
		while(q) {
			if(q->visited == 1) {
				q = q->nnext;
				continue;
			}
			r = q;
			n = q->dupsign;
			if(!n)
				q->visited = 1;
			while(n) {
				if(n->visited == 1) {
					n = n->dupsign;
					continue;
				}
				if(n->fsize < 16) {
					result = 0;
					count++;
				}
				else {
					gettimeofday(&start, NULL);
					result = comparefiles(r->mypath, n->mypath);
					gettimeofday(&stop, NULL);
					printf("time = %lu\n", stop.tv_usec - start.tv_usec);
				}
				if(result == 0) {
					n->visited = 1; 
					r->dup = n;
					r = n;
				}
				n = n->dupsign;
			}
		q = q->nnext;
		}
	p = p->mnext; 
	}
	printf("ccccccccccccccccccccccccccccccccccccccc=%d\n", count);
}
char *getsign(char *fname) {
	FILE *fp;
	int i;
	unsigned char c;
	char *sign =(char *)malloc(32 * 3 + 1), temp[4];
	if(sign == NULL) {
		printf("Out of Memory :\n");
		exit(1);
	}
	sign[0] = '\0';
	off_t fsize, x;
	fsize = filesize(fname);
	fp = fopen(fname, "rb");
	if(!fp) {
		perror("Open Failed\n");
		exit(errno);
	}
	if(fsize % 32 == 0)
		x = fsize / 32;
	else 
		x = fsize / 32 + 1;
	if(fsize < 16) {
		while(fread(&c, 1, 1, fp)) {
			snprintf(temp, 4, "%d", c);
			strcat(sign, temp);
		}
		fclose(fp);
		return sign;
	}
	for(i = 0; i < 16; i++) {
		fseek(fp, x * i, SEEK_SET);
		fread(&c, 1, 1, fp);
		snprintf(temp, 4, "%d", c);
		strcat(sign, temp);
		fseek(fp, (fsize - 2) - (x * i), SEEK_SET);
		fread(&c, 1, 1, fp);
		snprintf(temp, 4, "%d", c);
		strcat(sign, temp);	 
	}
	fclose(fp);
	return sign;
}
off_t filesize(char *filename) {
  	struct stat s;
 	if (stat(filename, &s) != 0) 
 	 	return -1;
 	return s.st_size;
}
int comparefiles(char *file1, char *file2) {
	FILE *f1, *f2;
	unsigned char c1, c2;
	f1 = fopen(file1, "rb");
	if(f1 == NULL) {
		perror("Open Failed\n");
		exit(errno);
	}
	f2 = fopen(file2, "rb");
	if(f2 == NULL) {
		perror("Open Failed\n");
		exit(errno);
	}
	while(fread(&c1, sizeof(c1), 1, f1) && fread(&c2, sizeof(c1), 1, f2)) {
		if(c1 != c2) {
			fclose(f1);
			fclose(f2);
			return 1;
		}	
	}
	fclose(f1);
	fclose(f2);
	return 0;
}
