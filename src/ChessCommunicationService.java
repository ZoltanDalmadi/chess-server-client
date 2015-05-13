import javafx.concurrent.Service;
import javafx.concurrent.Task;

public class ChessCommunicationService extends Service<Void> {

  ChessSocket chessSocket;

  public ChessCommunicationService(ChessSocket chessSocket) {
    this.chessSocket = chessSocket;
  }

  @Override
  protected Task<Void> createTask() {
    return new ChessCommunicationTask(chessSocket);
  }

}
