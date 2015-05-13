import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class ChessSocket implements AutoCloseable {

  Socket socket;
  PrintWriter out;
  BufferedReader in;
  ChessClientApp app;

  public ChessSocket(String host, int port, ChessClientApp chessClientApp) throws IOException {
    this.socket = new Socket(host, port);
    this.out = new PrintWriter(socket.getOutputStream(), true);
    this.in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
    this.app = chessClientApp;
  }

  @Override
  public void close() throws IOException {
    in.close();
    out.close();
    socket.close();
  }
}
