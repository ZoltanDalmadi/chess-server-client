import javafx.application.Platform;
import javafx.concurrent.Task;

public class ChessCommunicationTask extends Task<Void> {

  ChessSocket chessSocket;
  String line;

  public ChessCommunicationTask(ChessSocket chessSocket) {
    this.chessSocket = chessSocket;
  }

  @Override
  protected Void call() throws Exception {
    while ((line = chessSocket.in.readLine()) != null) {
      if (isCancelled())
        break;

      onMessage(line);
    }

    return null;
  }

  private void onMessage(String line) {
    Platform.runLater(() -> chessSocket.app.onMessage(line));
  }
}
