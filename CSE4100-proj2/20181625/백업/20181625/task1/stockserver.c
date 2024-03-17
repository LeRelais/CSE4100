/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
typedef struct _item {
    int id;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
} item;

typedef struct _node{
    struct _item *stock;
    struct _node *left;
    struct _node *right;
}node;

typedef struct _pool {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

node *root = NULL;
char string[10001];
sem_t smutex;
sem_t connmutex;
sem_t umutex;
int clientcnt;
int stringcnt;
char ansstring[101][MAXLINE];

void echo(int connfd);
void addnode(node **root, node *tmp);
void print();
node *search(node *root, int id);
void init_string();
void update();
void updatefile(FILE *f, node *root);
void check_clients(pool *p);

void init_string(){
    for(int i = 0; i < 101; i++){
        memset(ansstring[i], 0, sizeof(ansstring[i]));
    }
    stringcnt = 0;
}

void addnode(node **root, node *tmp){
    if(*root == NULL){
        *root = tmp;
        return;
    }

    if(tmp->stock->id > (*root)->stock->id){
        if((*root)->right == NULL)
        {
            (*root)->right = tmp;
            return;
        }
        addnode(&((*root)->right), tmp);
    }

     if(tmp->stock->id < (*root)->stock->id){
        if((*root)->left == NULL)
        {
            (*root)->left = tmp;
            return;
        }
        addnode(&((*root)->left), tmp);
    }
}

node *search(node *root, int id){
    if(root == NULL)
        return NULL;
    if(root->stock->id == id) return root;

    if(root->stock->id > id){
        search(root->left, id);
    }
    else{
        search(root->right, id);
    }
}

void traverse(node *root){
    if(root == NULL)
        return;
        
    sprintf(string, "%d %d %d\r\n", root->stock->id, root->stock->left_stock, root->stock->price);
    strcpy(ansstring[stringcnt++], string);
    root->stock->readcnt++;
    traverse(root->left);
    traverse(root->right);
}

void tree_traverse(){
    traverse(root);
}

void updatefile(FILE *f, node *root){
    if(root == NULL)
        return;
   
    updatefile(f, root->left);
    fprintf(f, "%d %d %d\n", root->stock->id, root->stock->left_stock, root->stock->price);    
    updatefile(f, root->right);
}

void init_pool(int listenfd, pool *p){
    p->maxi = -1;
    for(int i = 0; i < FD_SETSIZE; i++){
        p->clientfd[i] = -1;
    }
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    p->nready--;
    int i = 0;
    for(i = 0; i < FD_SETSIZE; i++){
        if(p->clientfd[i] < 0){
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if(connfd > p->maxfd)
                p->maxfd = connfd;

            if(i > p->maxi)
                p->maxi = i;

            clientcnt++;
            break;
        }
    }

    if(i == FD_SETSIZE)
        app_error("add_client error: Too many clients");
}

void check_clients(pool *p){
    int connfd, n;
    char buf[MAXLINE];
    char ans[MAXLINE];
    rio_t rio;

    fprintf(stdout, "check_clients() called\n");
    for(int i = 0; i <= p->maxi && p->nready > 0; i++){
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if(connfd > 0 && FD_ISSET(connfd, &p->ready_set)){
            p->nready--;
            if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
                buf[strlen(buf)-1] = ' ';
                fprintf(stdout, "message |%s| received\n", buf);

                char *tmp = buf;
                char *delim;
                char *argv[MAXLINE];
                int argument_count = 0;

                while((delim = strchr(tmp, ' '))){
                    argv[argument_count++] = tmp;
                    *delim = '\0';
                    tmp = delim + 1;
                }
                argv[argument_count] = tmp;
                argument_count++;

                if(!strcmp(argv[0], "show")){
                   P(&smutex);
                    strcpy(string, ""); ////////
                    init_string();
                    tree_traverse();
                    strcpy(string, ansstring[0]);
                    for(int i = 1; i < stringcnt; i++)
                        strcat(string, ansstring[i]);
                    Rio_writen(connfd, string, MAXLINE);
                    V(&smutex);
                }

                else if(!strcmp(argv[0], "buy")){
                    int cnt = atoi(argv[2]);
                    int id = atoi(argv[1]);
                    node *ntmp = search(root, id);
                    
                    if(ntmp == NULL){
                        sprintf(ans, "[buy] Not enough left stock\n");
                        Rio_writen(connfd, ans, MAXLINE);
                        continue;
                    }

                    ntmp->stock->readcnt++;

                    P(&(ntmp->stock->mutex));
                    if(ntmp->stock->left_stock >= cnt){
                        ntmp->stock->left_stock -= cnt;
                        sprintf(ans, "[buy] success\n");
                    }
                    else
                        sprintf(ans, "[buy] Not enough stock is left\n");
                        
                    V(&(ntmp->stock->mutex));
                    Rio_writen(connfd, ans, MAXLINE);
                }

                else if(!strcmp(argv[0], "sell")){
                    int cnt = atoi(argv[2]);
                    int id = atoi(argv[1]);
                    node *ntmp = search(root, id);

                    if(ntmp == NULL){ 
                        ntmp = (node*)malloc(sizeof(node));
                        ntmp->stock = (item*)malloc(sizeof(item));
                        ntmp->stock->id = id;
                        ntmp->stock->left_stock = cnt;
                        ntmp->stock->price = 5001;
                        ntmp->stock->readcnt = 0;
                        sem_init(&ntmp->stock->mutex, 0, 1);
                        ntmp->left = NULL;
                        ntmp->right = NULL;
                        addnode(&root, ntmp);
                    }

                    ntmp->stock->readcnt++;

                    P(&(ntmp->stock->mutex));
                    ntmp->stock->left_stock += cnt;
                    sprintf(ans, "[sell] success\n");
                    V(&(ntmp->stock->mutex));
                    Rio_writen(connfd, ans, MAXLINE);
                }

                else if(!strcmp(argv[0], "exit")){
                    sprintf(ans, "terminate connection\n");
                    Rio_writen(connfd, ans, MAXLINE);
                    Close(connfd);
                    FD_CLR(connfd, &(p->read_set));
                    p->clientfd[i] = -1;

                    P(&connmutex);
                    clientcnt--;
                     if(!(clientcnt > 0)){
                        P(&umutex);
                        FILE *f = fopen("stock.txt", "w");
                        updatefile(f, root);
                        fclose(f);
                        V(&umutex);
                    }
                    V(&connmutex);
                }
                else
                    continue;
            }
            else{
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;

                P(&connmutex);
                clientcnt--;
                if(!(clientcnt > 0)){
                        P(&umutex);
                        FILE *f = fopen("stock.txt", "w");
                        updatefile(f, root);
                        fclose(f);
                        V(&umutex);
                    }
                V(&connmutex);
            }
        }
    }

    fprintf(stdout, "check_clients() end\n");
}

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool Pool;

    FILE *f = fopen("stock.txt", "r");
    if(f == NULL){
        printf("file open error\n");
    }

    int stock_id = 0, num_left = 0, price = 0, num_tree = 0;

    while(fscanf(f, "%d %d %d", &stock_id, &num_left, &price) != EOF){
        node *tmp = (node*)malloc(sizeof(node));
        tmp->stock = (item*)malloc(sizeof(item));
        tmp->stock->id = stock_id;
        tmp->stock->readcnt = 0;
        tmp->stock->left_stock = num_left;
        tmp->stock->price = price;
        tmp->left = NULL;
        tmp->right = NULL;
        sem_init(&(tmp->stock->mutex), 0, 1);
        addnode(&root, tmp);
        num_tree++;
    }

    fprintf(stdout, "%d\n", num_tree);
    
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    sem_init(&smutex, 0, 1);
    sem_init(&connmutex, 0, 1);
    sem_init(&umutex, 0, 1);
    listenfd = Open_listenfd(argv[1]);

    init_pool(listenfd, &Pool);
   
    //fprintf(stdout, "1\n");
    while (1) {
        Pool.ready_set = Pool.read_set;
        Pool.nready = Select(Pool.maxfd+1, &(Pool.ready_set), NULL, NULL, NULL);
         //fprintf(stdout, "1\n");
	    if(FD_ISSET(listenfd, &(Pool.ready_set))){
            clientlen = sizeof(struct sockaddr_storage); 
	        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
            add_client (connfd, &Pool);
             //fprintf(stdout, "1\n");
        }
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
	    //Close(connfd);
        //echo(connfd);
        check_clients(&Pool);
    }
    exit(0);
}
/* $end echoserverimain */
