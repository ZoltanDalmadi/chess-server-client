#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define PORT_NO 2001
#define error(a, b) fprintf(stderr, a, b)

int chess_table[8][8];
char buffer[BUF_SIZE + 1];

typedef enum piece_enum
{
  WHITE_PAWN = 1,
  WHITE_BISHOP = 2,
  WHITE_KNIGHT = 3,
  WHITE_ROOK = 4,
  WHITE_QUEEN = 5,
  WHITE_KING = 6,
  BLACK_PAWN = 7,
  BLACK_BISHOP = 8,
  BLACK_KNIGHT = 9,
  BLACK_ROOK = 10,
  BLACK_QUEEN = 11,
  BLACK_KING = 12
} PIECE;

typedef struct player_struct
{
  PIECE pieces[6];
  int socket;
  int lost;
} PLAYER;

PLAYER player1;
PLAYER player2;

void send_msg(PLAYER *player, char *msg)
{
  size_t bytes = strlen(msg) + 1;
  ssize_t tr_size = send(player->socket, msg, bytes, 0);

  if (tr_size < 0)
  {
    fprintf(stderr, "Cannot send data to the client.\n");
    exit(6);
  }
}

int valid_move(PLAYER *player, int from_row, int from_col,
               int to_row, int to_col)
{
  /* check if cell is empty */
  if (chess_table[from_row][from_col] == 0)
    return 0;

  /* check if player attempted to move with a piece of the other player */
  int found_own = 0;

  for (int i = 0; i < 6; ++i)
  {
    if (chess_table[from_row][from_col] == player->pieces[i])
      found_own = 1;
  }

  if (!found_own) return 0;

  /* check if destination cell is not occupied by another own piece */
  for (int i = 0; i < 6; ++i)
    if (chess_table[to_row][to_col] == player->pieces[i]) return 0;

  return 1;
}

int move_piece(PLAYER *player, int from_row, int from_col,
               int to_row, int to_col)
{
  if (valid_move(player, from_row, from_col, to_row, to_col))
  {
    int temp = chess_table[from_row][from_col];
    chess_table[from_row][from_col] = 0;

    if (chess_table[to_row][to_col] == WHITE_KING)
      player1.lost = 1;

    if (chess_table[to_row][to_col] == BLACK_KING)
      player2.lost = 1;

    chess_table[to_row][to_col] = temp;

    return 1;
  }
  else
  {
    snprintf(buffer, BUF_SIZE, "*ERR* Invalid move. Please try again.\n");
    send_msg(player, buffer);
    return 0;
  }
}

int parse_command(char *command, int *from_row, int *from_col, int *to_row,
                  int *to_col)
{
  char *from = strtok(command, ";");
  char *to = strtok(NULL, ";");
  char *from_col_s = strtok(from, ",");
  char *from_row_s = strtok(NULL, ",");
  char *to_col_s = strtok(to, ",");
  char *to_row_s = strtok(NULL, ",");

  /* convert chars to int and letters to corresponding indexes */
  *from_col = from_col_s[0] - 65;
  *from_row = from_row_s[0] - '0';
  *from_row -= 1;
  *to_col = to_col_s[0] - 65;
  *to_row = to_row_s[0] - '0';
  *to_row -= 1;

  return 1;
}

int receive_command(PLAYER *player)
{
  char command[255];
  ssize_t rcv_size = recv(player->socket, command, 255, 0);

  if (rcv_size < 0)
  {
    fprintf(stderr, "Cannot receive from the socket.\n");
    exit(5);
  }

  if (!strcmp(command, "I_GIVE_UP"))
  {
    player->lost = 1;
    return 1;
  }

  int from_row, from_col, to_row, to_col;

  if (!parse_command(command, &from_row, &from_col, &to_row, &to_col))
    return 0;

  if (!move_piece(player, from_row, from_col, to_row, to_col))
    return 0;

  return 1;
}

void print_table()
{
  for (int i = 7; i >= 0; --i)
    for (int j = 0; j < 8; ++j)
    {
      printf("%2d", chess_table[i][j]);
      printf(j == 7 ? " | %d\n" : " ", i + 1);
    }

  printf("-----------------------\n");
  printf(" A  B  C  D  E  F  G  H\n\n");
}

void send_table()
{
  memset(buffer, 0, BUF_SIZE + 1);
  strcat(buffer, "*TBL* ");

  char buf[4];

  for (int i = 0; i < 8; ++i)
  {
    for (int j = 0; j < 8; ++j)
    {
      sprintf(buf, "%d:", chess_table[i][j]);
      strcat(buffer, buf);
    }
  }

  buffer[strlen(buffer) - 1] = '\n';

  send_msg(&player1, buffer);
  send_msg(&player2, buffer);
}

int main(int argc, char *argv[])
{
  /* Declarations */
  int server_socket;                     // socket endpoint
  struct sockaddr_in server;             // socket name (address) of server
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(PORT_NO);

  struct sockaddr_in client;             // socket name of client
  socklen_t client_size = sizeof client; // length of the socket address client

  int err;                               // error code
  char on = 1;

  /* Creating socket */
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket < 0)
  {
    error("%s: Socket creation error\n", argv[0]);
    exit(1);
  }

  /* Setting socket options */
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
  setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);

  /* Binding socket */
  err = bind(server_socket, (struct sockaddr *) &server, sizeof server);

  if (err < 0)
  {
    error("%s: Cannot bind to the socket\n", argv[0]);
    exit(2);
  }

  /* Listening */
  err = listen(server_socket, 10);

  if (err < 0)
  {
    error("%s: Cannot listen to the socket\n", argv[0]);
    exit(3);
  }

  puts("Waiting for players to connect.");

  /* ----------------------------------------------------------------------- */

  player1.pieces[0] = WHITE_PAWN;
  player1.pieces[1] = WHITE_ROOK;
  player1.pieces[2] = WHITE_KNIGHT;
  player1.pieces[3] = WHITE_BISHOP;
  player1.pieces[4] = WHITE_QUEEN;
  player1.pieces[5] = WHITE_KING;
  player1.lost = 0;

  player1.socket
    = accept(server_socket, (struct sockaddr *) &client, &client_size);

  if (player1.socket < 0)
  {
    error("%s : Cannot accept on socket\n", argv[0]);
    exit(4);
  }

  printf("Player 1 connected.\n\nWaiting for Player 2.\n\n");

  snprintf(buffer, BUF_SIZE, "*MSG* Please wait for Player 2 to join.\n");
  send_msg(&player1, buffer);

  player2.pieces[0] = BLACK_PAWN;
  player2.pieces[1] = BLACK_ROOK;
  player2.pieces[2] = BLACK_KNIGHT;
  player2.pieces[3] = BLACK_BISHOP;
  player2.pieces[4] = BLACK_QUEEN;
  player2.pieces[5] = BLACK_KING;
  player2.lost = 0;

  player2.socket
    = accept(server_socket, (struct sockaddr *) &client, &client_size);

  if (player2.socket < 0)
  {
    error("%s : Cannot accept on socket\n", argv[0]);
    exit(4);
  }

  puts("Player 2 connected.\n");

  /* ----------------------------------------------------------------------- */

  /* start game */
  puts("The game has started.");

  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      chess_table[i][j] = 0;

  for (int i = 0; i < 8; ++i)
    chess_table[1][i] = WHITE_PAWN;

  chess_table[0][0] = WHITE_ROOK;
  chess_table[0][1] = WHITE_KNIGHT;
  chess_table[0][2] = WHITE_BISHOP;
  chess_table[0][3] = WHITE_QUEEN;
  chess_table[0][4] = WHITE_KING;
  chess_table[0][5] = WHITE_BISHOP;
  chess_table[0][6] = WHITE_KNIGHT;
  chess_table[0][7] = WHITE_ROOK;

  for (int i = 0; i < 8; ++i)
    chess_table[6][i] = BLACK_PAWN;

  chess_table[7][0] = BLACK_ROOK;
  chess_table[7][1] = BLACK_KNIGHT;
  chess_table[7][2] = BLACK_BISHOP;
  chess_table[7][3] = BLACK_QUEEN;
  chess_table[7][4] = BLACK_KING;
  chess_table[7][5] = BLACK_BISHOP;
  chess_table[7][6] = BLACK_KNIGHT;
  chess_table[7][7] = BLACK_ROOK;

  print_table();

  /* main loop */

  while (1)
  {
    send_table();
    print_table();

    snprintf(buffer, BUF_SIZE, "*MOV* Your turn.\n");
    send_msg(&player1, buffer);

    snprintf(buffer, BUF_SIZE,
             "*WAIT* Please wait for your opponent's turn.\n");
    send_msg(&player2, buffer);

    while (1)
    {
      if (receive_command(&player1))
        break;
    }

    if (player1.lost)
    {
      snprintf(buffer, BUF_SIZE,
               "*END* Congratulations, You have won the game!\n");
      send_msg(&player2, buffer);

      snprintf(buffer, BUF_SIZE, "*END* You lose. :(\n");
      send_msg(&player1, buffer);
      break;
    }

    if (player2.lost)
    {
      snprintf(buffer, BUF_SIZE,
               "*END* Congratulations, You have won the game!\n");
      send_msg(&player1, buffer);

      snprintf(buffer, BUF_SIZE, "*END* You lose. :(\n");
      send_msg(&player2, buffer);
      break;
    }

    send_table();
    print_table();

    snprintf(buffer, BUF_SIZE, "*MOV* Your turn.\n");
    send_msg(&player2, buffer);

    snprintf(buffer, BUF_SIZE,
             "*WAIT* Please wait for your opponent's turn.\n");
    send_msg(&player1, buffer);

    while (1)
    {
      if (receive_command(&player2))
        break;
    }

    if (player1.lost)
    {
      snprintf(buffer, BUF_SIZE,
               "*END* Congratulations, You have won the game!\n");
      send_msg(&player2, buffer);

      snprintf(buffer, BUF_SIZE, "*END* You lose. :(\n");
      send_msg(&player1, buffer);
      break;
    }

    if (player2.lost)
    {
      snprintf(buffer, BUF_SIZE,
               "*END* Congratulations, You have won the game!\n");
      send_msg(&player1, buffer);

      snprintf(buffer, BUF_SIZE, "*END* You lose. :(\n");
      send_msg(&player2, buffer);
      break;
    }

  }

  /* Closing sockets and quit */
  close(player2.socket);
  close(player1.socket);
  close(server_socket);
  exit(0);
}
