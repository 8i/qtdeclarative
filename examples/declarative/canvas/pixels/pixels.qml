/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "../contents"
Item {
  id:container
  width:360
  height:600

  Column {
    spacing:5
    anchors.fill:parent
    Text { font.pointSize:25; text:"Processing pixels"; anchors.horizontalCenter:parent.horizontalCenter}

    Canvas {
      id:canvas
      width:360
      height:360
      smooth:true
      renderTarget:Canvas.FramebufferObject
      renderInThread:false
      property string image :"../contents/qt-logo.png"
      Component.onCompleted:loadImage(image);
      onImageLoaded:requestPaint();
    onPaint: {
      
      var ctx = canvas.getContext('2d');
      if (canvas.isImageLoaded(image)) {
          var pixels = ctx.createImageData(image);
          //pixels.mirror();
          pixels.filter(Canvas.GrayScale);
          //pixels.filter(Canvas.Threshold, 100); //default 127
          //pixels.filter(Canvas.Blur, 20); //default 10
          //pixels.filter(Canvas.Opaque);
          //pixels.filter(Canvas.Invert);
          //pixels.filter(Canvas.Convolute, [0,-1,0,
          //                                 -1,5,-1,
          //                                 0,-1,0]);
          //ctx.putImageData(pixels, 0, 0, canvas.width, canvas.height);
          ctx.putImageData(pixels, 0, 0);
      }
    }
  }
 }
}
