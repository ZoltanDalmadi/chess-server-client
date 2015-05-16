import javafx.application.Application;
import javafx.application.Platform;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.control.Menu;
import javafx.scene.control.MenuBar;
import javafx.scene.control.MenuItem;
import javafx.scene.image.Image;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.FlowPane;
import javafx.scene.layout.GridPane;
import javafx.scene.text.Font;
import javafx.stage.Stage;

import java.io.IOException;

public class ChessClientApp extends Application {

  StringBuffer commandBuffer;
  boolean commandInProgress;
  ChessCell[][] chessCells = new ChessCell[8][8];
  ChessCell pieceToMove;
  boolean canMove = true;
  ChessSocket socket;
  ChessCommunicationService communicationService;
  Label statusText;
  GridPane board;

  EventHandler<MouseEvent> cellHandler = e -> {
    if (canMove) {
      ChessCell currentChessCell = (ChessCell) e.getSource();

      if (commandInProgress) {
        commandBuffer.append(";");
        commandInProgress = false;
      } else {
        commandBuffer.setLength(0);
        commandInProgress = true;
        currentChessCell.setBorder(ChessCell.pieceToMoveBorder);
        pieceToMove = currentChessCell;
      }

      commandBuffer.append(currentChessCell.toString());

      if (!commandInProgress) {
        socket.out.println(commandBuffer.toString());
        pieceToMove = null;
        resetBorders();
      }
    }
  };

  EventHandler<MouseEvent> hoverHandler = e -> {
    if (canMove) {
      ChessCell currentChessCell = (ChessCell) e.getSource();
      if (commandInProgress && !currentChessCell.equals(pieceToMove)) {
        currentChessCell.setBorder(ChessCell.cellHighLightBorder);
      }
    }
  };

  EventHandler<MouseEvent> unHoverHandler = e -> {
    if (canMove) {
      ChessCell currentChessCell = (ChessCell) e.getSource();
      if (commandInProgress && !currentChessCell.equals(pieceToMove)) {
        currentChessCell.setBorder(null);
      }
    }
  };

  public static void main(String[] args) {
    launch(args);
  }

  private void resetBorders() {
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 8; j++)
        chessCells[i][j].setBorder(null);
  }

  void updateBoard(String board) {
    String[] strings = board.split(":");

    int k = 0;

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        chessCells[i][j].changeTo(Integer.parseInt(strings[k++]));
      }
    }
  }

  @Override
  public void start(Stage primaryStage) {
    BorderPane root = new BorderPane();

    MenuBar menuBar = new MenuBar();
    Menu serverMenu = new Menu("Server");
    MenuItem connectMenuItem = new MenuItem("Connect to server");
    MenuItem exitMenuItem = new MenuItem("Exit");
    connectMenuItem.setOnAction(e -> connect());
    exitMenuItem.setOnAction(e -> Platform.exit());
    serverMenu.getItems().addAll(connectMenuItem, exitMenuItem);
    Menu gameMenu = new Menu("Game");
    MenuItem giveUpItem = new MenuItem("Give up");
    giveUpItem.setOnAction(e -> giveUp());
    gameMenu.getItems().add(giveUpItem);
    menuBar.getMenus().addAll(serverMenu, gameMenu);

    board = new GridPane();

    FlowPane statusBar = new FlowPane();
    statusText = new Label();
    statusText.setFont(new Font(20));
    statusBar.getChildren().add(statusText);
    statusBar.setPadding(new Insets(5));

    root.setTop(menuBar);
    root.setCenter(board);
    root.setBottom(statusBar);
    Scene scene = new Scene(root);

    Image sprite = new Image("/sprite.png");

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        chessCells[i][j] = new ChessCell(sprite);
        chessCells[i][j].setCellColor(i % 2 == j % 2);
        chessCells[i][j].row = i;
        chessCells[i][j].col = (char) (j + 65);
        chessCells[i][j].setOnMouseClicked(cellHandler);
        chessCells[i][j].setOnMouseEntered(hoverHandler);
        chessCells[i][j].setOnMouseExited(unHoverHandler);
      }
    }

    for (int i = 0; i < 8; i++) {
      board.addRow(i,
          chessCells[7 - i][0], chessCells[7 - i][1], chessCells[7 - i][2], chessCells[7 - i][3],
          chessCells[7 - i][4], chessCells[7 - i][5], chessCells[7 - i][6], chessCells[7 - i][7]);
    }

    board.setRotate(180);
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        chessCells[i][j].setRotate(-180);
      }
    }

    commandBuffer = new StringBuffer();

    primaryStage.setTitle("Chess client");
    primaryStage.setScene(scene);
    primaryStage.show();
  }

  private void giveUp() {
    commandBuffer.setLength(0);
    commandBuffer.append("I_GIVE_UP");
    socket.out.println(commandBuffer.toString());
  }

  private void rotateBoardBack() {
    board.setRotate(0);
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        chessCells[i][j].setRotate(0);
      }
    }
  }

  public void connect() {
    try {
      socket = new ChessSocket("localhost", 2001, this);
      communicationService = new ChessCommunicationService(socket);
      communicationService.start();
    } catch (IOException e) {
      statusText.setStyle("-fx-text-fill: red");
      statusText.setText("Could not connect to server.");
    }
  }

  public void closeConnection() {
    try {
      socket.close();
      communicationService.cancel();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public void onMessage(String message) {
    if (message.contains("*TBL*")) {
      updateBoard(message.substring(message.indexOf("*") + 6));
    }

    if (message.contains("*MSG*")) {
      statusText.setStyle("-fx-text-fill: blue");
      statusText.setText(message.substring(message.indexOf("*") + 6));
      rotateBoardBack();
    }

    if (message.contains("*ERR*")) {
      statusText.setStyle("-fx-text-fill: red");
      statusText.setText(message.substring(message.indexOf("*") + 6));
    }

    if (message.contains("*MOV*")) {
      statusText.setStyle("-fx-text-fill: green");
      statusText.setText(message.substring(message.indexOf("*") + 6));
      canMove = true;
    }

    if (message.contains("*WAIT*")) {
      statusText.setStyle("-fx-text-fill: darkorange");
      statusText.setText(message.substring(message.indexOf("*") + 7));
      canMove = false;
    }

    if (message.contains("*END*")) {
      statusText.setStyle("-fx-text-fill: purple");
      statusText.setText(message.substring(message.indexOf("*") + 6));
      closeConnection();
    }

  }
}
