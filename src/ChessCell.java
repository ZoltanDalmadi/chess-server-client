import javafx.geometry.Rectangle2D;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.shape.StrokeType;

public class ChessCell extends Pane {

  private static BorderStrokeStyle cellBorderStyle =
      new BorderStrokeStyle(StrokeType.OUTSIDE, null, null, 10, 0, null);
  public static Border pieceToMoveBorder =
      new Border(new BorderStroke(Color.RED, cellBorderStyle, null, new BorderWidths(5)));
  public static Border cellHighLightBorder =
      new Border(new BorderStroke(Color.DARKORANGE, cellBorderStyle, null, new BorderWidths(5)));

  private static BackgroundFill darkFill = new BackgroundFill(Color.DARKGRAY, null, null);
  private static Background darkBackground = new Background(darkFill);
  private static BackgroundFill lightFill = new BackgroundFill(Color.WHITE, null, null);
  private static Background lightBackground = new Background(lightFill);
  int row;
  char col;
  private ImageView pieceDisplay = new ImageView();

  public ChessCell(Image image) {
    pieceDisplay.setImage(image);
    pieceDisplay.setViewport(new Rectangle2D(0, 0, 64, 64));
    getChildren().add(pieceDisplay);
  }

  @Override
  public String toString() {
    return String.valueOf(col) + "," + (row + 1);
  }

  public void setCellColor(boolean dark) {
    setBackground(dark ? darkBackground : lightBackground);
  }

  public void changeTo(int offset) {
    pieceDisplay.setViewport(new Rectangle2D(offset * 64, 0, 64, 64));
  }

}
