#include "DropdownList.h"
#include "TextField.h"

#include <string.h>

#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"



// start:  none focused
DropdownList *DropdownList::sFocusedDropdownList = NULL;

extern double frameRateFactor;

int DropdownList::sDeleteFirstDelaySteps = 30 / frameRateFactor;
int DropdownList::sDeleteNextDelaySteps = 2 / frameRateFactor;

extern Font *oldMainFont;



DropdownList::DropdownList( Font *inDisplayFont, 
                      double inX, double inY, int inCharsWide,
                      char inForceCaps,
                      const char *inLabelText,
                      const char *inAllowedChars,
                      const char *inForbiddenChars )
        : PageComponent( inX, inY ),
          mActive( true ),
          mContentsHidden( false ),
           
          mHiddenSprite( loadSprite( "hiddenFieldTexture.tga", false ) ),
          mFont( inDisplayFont ), 
          mCharsWide( inCharsWide ),
          mMaxLength( -1 ),
          mFireOnAnyChange( false ),
          mFireOnLeave( false ),
          mForceCaps( inForceCaps ),
          mLabelText( NULL ),
          mAllowedChars( NULL ), mForbiddenChars( NULL ),
          
          mHover( false ),
		  
		  mRawText( new unicode[1] ),
		  hoverIndex( -1 ),
		  nearRightEdge( 0 ),
		  mUseClearButton( false ),
		  onClearButton( false ),
		  
          mFocused( false ), mText( new unicode[1] ),
          mTextLen( 0 ),
          mCursorPosition( 0 ),
          mIgnoreArrowKeys( false ),
          mIgnoreMouse( false ),
          mDrawnText( NULL ),
          mCursorDrawPosition( 0 ),
          mHoldDeleteSteps( -1 ), mFirstDeleteRepeatDone( false ),
          mLabelOnRight( false ),
          mLabelOnTop( false ),
          mSelectionStart( -1 ),
          mSelectionEnd( -1 ),
          mShiftPlusArrowsCanSelect( false ),
          mCursorFlashSteps( 0 ),
          mUsePasteShortcut( false ) {
    
    if( inLabelText != NULL ) {
        //mLabelText = stringDuplicate( inLabelText );
        mLabelText = new unicode[strlen(inLabelText) + 1];
        utf8ToUnicode(inLabelText, mLabelText);
    }
    
    if( inAllowedChars != NULL ) {
        //mAllowedChars = stringDuplicate( inAllowedChars );
        mAllowedChars = new unicode[strlen(inAllowedChars) + 1];
        utf8ToUnicode(inAllowedChars, mAllowedChars);
    }
    if( inForbiddenChars != NULL ) {
        //mForbiddenChars = stringDuplicate( inForbiddenChars );
        mForbiddenChars = new unicode[strlen(inForbiddenChars) + 1];
        utf8ToUnicode(inForbiddenChars, mForbiddenChars);
    }

    clearArrowRepeat();
        

    mCharWidth = mFont->getFontHeight();

    mBorderWide = mCharWidth * 0.25;

    mHigh = mFont->getFontHeight() + 2 * mBorderWide;

    unicode *fullString = new unicode[ mCharsWide + 1 ];

    unicode widestChar = 0;
    double width = 0;

    unicode* measureText = L"你好，世界！";
    for (int i=0; i<6; i++){
        unicode s[2];
        s[0] = measureText[i];
        s[1] = 0;
        double thisWidth = mFont->measureString( s );
            
        if( thisWidth > width ) {
            width = thisWidth;
            widestChar = measureText[i];    
        }
    }
    /*
    for( int c=32; c<128; c++ ) {
        unicode pc = processCharacter( c );

        if( pc != 0 ) {
            unicode s[2];
            s[0] = pc;
            s[1] = 0;

            double thisWidth = mFont->measureString( s );
            
            if( thisWidth > width ) {
                width = thisWidth;
                widestChar = pc;    
            }
        }
    }
    */
    


    for( int i=0; i<mCharsWide; i++ ) {
        fullString[i] = widestChar;
    }
    fullString[ mCharsWide ] =  0;
    
    double fullStringWidth = mFont->measureString( fullString );

    delete [] fullString;

    mWide = fullStringWidth + 2 * mBorderWide;
    
    mDrawnTextX = - ( mWide / 2 - mBorderWide );

    mText[0] = 0;
    }



DropdownList::~DropdownList() {
    if( this == sFocusedDropdownList ) {
        // we're focused, now nothing is focused
        sFocusedDropdownList = NULL;
        }

    delete [] mText;

    if( mLabelText != NULL ) {
        delete [] mLabelText;
        }

    if( mAllowedChars != NULL ) {
        delete [] mAllowedChars;
        }
    if( mForbiddenChars != NULL ) {
        delete [] mForbiddenChars;
        }

    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }

    if( mHiddenSprite != NULL ) {
        freeSprite( mHiddenSprite );
        }
    }



void DropdownList::setContentsHidden( char inHidden ) {
    mContentsHidden = inHidden;
    }




void DropdownList::setList( const unicode *inText ) {
    delete [] mRawText;

    // obeys same rules as typing (skip blocked characters)
    int length = wcslen(inText);
    unicode* filteredText = new unicode[length+1];
    int index = 0;
    for( int i=0; i< length; i++ ) {
        unicode processedChar = processCharacter( inText[i] );
        
        if( processedChar != 0 ) {
            filteredText[index++] = processedChar ;
        }
    }
    filteredText[index] = 0;
	int numLines;
	unicode **lines = split( filteredText, (unicode)"\n", &numLines );
	
	mRawText = filteredText;
	
    removeExtraNewlinesAndWhiteSpace(mRawText);
	int firstLineIndex = 0;
    listLen = 0;
    for (int i=0; mRawText[i] != 0; i++) {
        if (mRawText[i] == (unicode)'\n') {
            listLen ++;
            if (firstLineIndex == 0){
                firstLineIndex = i;
            }
        }
    }
    listLen += 1; // 一共有这么多行文本

    // 设置mText为第一行文本内容
    delete [] mText;
    mText = new unicode [firstLineIndex+1];
    memcpy(mText, mRawText, (firstLineIndex - 1)*sizeof(unicode));
    mText[firstLineIndex] = '\0';
}


// 将mText放在mRawText开头
unicode *DropdownList::getAndUpdateList() {
	
	unicode* result = wcsstr(mRawText, mText);
    if (result != NULL){
        int d = result - mRawText;
        rotateUnicodeString(mRawText, d);
    }
	return stringDuplicate( mRawText );
}
	
	
void DropdownList::setText( const unicode *inText ) {
    delete [] mText;
    
    mSelectionStart = -1;
    mSelectionEnd = -1;
    
	mText = stringDuplicate( inText );
	
    mTextLen = wcslen( mText );
    
    mCursorPosition = mTextLen;

    // hold-downs broken
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();
}
	
unicode *DropdownList::getText() {
	return stringDuplicate( mText );
}
	
void DropdownList::selectOption( int index ) {	
	if( index < 0 || index >= listLen ) return;
	unicode* start = mRawText;
    while (--index > 0) {
        start = wcsstr(start, L"\n");
        if (start == NULL) {
            printf("Error in DropDownList::selectOption: index(%d) listLen(%d)\n", index, listLen);
            exit(-1);
        }
    }
    unicode* end = wcsstr(start, L"\n");
    if (end == NULL) {
        setText(start);
    } else {
        delete [] mText;
        int length = end - start+1;
        mText = new unicode[length];
        memcpy(mText, start, sizeof(unicode)*(length-1));
        mText[length] = 0;
    }
}
	
void DropdownList::deleteOption( int index ) {
	if( index < 0 || index >= listLen ) return;
	unicode* start = mRawText;
    while (--index > 0) {
        start = wcsstr(start, L"\n");
        if (start == NULL) {
            printf("Error in DropDownList::selectOption: index(%d) listLen(%d)\n", index, listLen);
            exit(-1);
        }
    }
    unicode* end = wcsstr(start, L"\n");
	if (end == NULL) {
        mRawText[start] = 0;
    } else {
        int length = wcslen(mRawText);
        int size = length - (end - mRawText);
        memmove(start, end+1, sizeof(unicode)*size)
    }
	listLen = listLen - 1;
}



void DropdownList::setMaxLength( int inLimit ) {
    mMaxLength = inLimit;
    }



int DropdownList::getMaxLength() {
    return mMaxLength;
    }



char DropdownList::isAtLimit() {
    if( mMaxLength == -1 ) {
        return false;
        }
    else {
        return ( mTextLen == mMaxLength );
        }
    }
    



void DropdownList::setActive( char inActive ) {
    mActive = inActive;
    }



char DropdownList::isActive() {
    return mActive;
    }
        


void DropdownList::step() {

    mCursorFlashSteps ++;

    if( mHoldDeleteSteps > -1 ) {
        mHoldDeleteSteps ++;

        int stepsBetween = sDeleteFirstDelaySteps;
        
        if( mFirstDeleteRepeatDone ) {
            stepsBetween = sDeleteNextDelaySteps;
            }
        
        if( mHoldDeleteSteps > stepsBetween ) {
            // delete repeat
            mHoldDeleteSteps = 0;
            mFirstDeleteRepeatDone = true;
            
            deleteHit();
            }
        }


    for( int i=0; i<2; i++ ) {
        
        if( mHoldArrowSteps[i] > -1 ) {
            mHoldArrowSteps[i] ++;

            int stepsBetween = sDeleteFirstDelaySteps;
        
            if( mFirstArrowRepeatDone[i] ) {
                stepsBetween = sDeleteNextDelaySteps;
                }
        
            if( mHoldArrowSteps[i] > stepsBetween ) {
                // arrow repeat
                mHoldArrowSteps[i] = 0;
                mFirstArrowRepeatDone[i] = true;
            
                switch( i ) {
                    case 0:
                        leftHit();
                        break;
                    case 1:
                        rightHit();
                        break;
                    }
                }
            }
        }


    }

        
        
void DropdownList::draw() {
    
    if( mFocused ) {    
        setDrawColor( 1, 1, 1, 1 );
        }
    else {
        setDrawColor( 0.5, 0.5, 0.5, 1 );
        }
    

    drawRect( - mWide / 2, - mHigh / 2, 
              mWide / 2, mHigh / 2 );
    
    setDrawColor( 0.25, 0.25, 0.25, 1 );
    double pixWidth = mCharWidth / 8;


    double rectStartX = - mWide / 2 + pixWidth;
    double rectStartY = - mHigh / 2 + pixWidth;

    double rectEndX = mWide / 2 - pixWidth;
    double rectEndY = mHigh / 2 - pixWidth;

    double middleWidth = mWide - 2 * pixWidth;
    
    drawRect( rectStartX, rectStartY,
              rectEndX, rectEndY );
    
    setDrawColor( 1, 1, 1, 1 );

    if( mContentsHidden && mHiddenSprite != NULL ) {
        startAddingToStencil( false, true );

        drawRect( rectStartX, rectStartY,
                  rectEndX, rectEndY );
        startDrawingThroughStencil();
        
        doublePair pos = { 0, 0 };
        
        drawSprite( mHiddenSprite, pos );
        
        stopStencil();
        }
    


    
    if( mLabelText != NULL ) {
        TextAlignment a = alignRight;
        double xPos = -mWide/2 - mBorderWide;
        
        double yPos = 0;
        
        if( mLabelOnTop ) {
            xPos += mBorderWide + pixWidth;
            yPos = mHigh / 2 + 2 * mBorderWide;
            }

        if( mLabelOnRight ) {
            a = alignLeft;
            xPos = -xPos;
            }
        
        if( mLabelOnTop ) {
            // reverse align if on top
            if( a == alignLeft ) {
                a = alignRight;
                }
            else {
                a = alignLeft;
                }
            }
        
        doublePair labelPos = { xPos, yPos };
        
        mFont->drawString( mLabelText, labelPos, a );
        }
    
    
    if( mContentsHidden ) {
        return;
        }




    doublePair textPos = { - mWide/2 + mBorderWide, 0 };


    char tooLongFront = false;
    char tooLongBack = false;
    
    mCursorDrawPosition = mCursorPosition;


    unicode *textBeforeCursorBase = stringDuplicate( mText );
    unicode *textAfterCursorBase = stringDuplicate( mText );
    
    unicode *textBeforeCursor = textBeforeCursorBase;
    unicode *textAfterCursor = textAfterCursorBase;

    textBeforeCursor[ mCursorPosition ] = 0;
    
    textAfterCursor = &( textAfterCursor[ mCursorPosition ] );

    if( mFont->measureString( mText ) > mWide - 2 * mBorderWide ) {
        
        if( mFont->measureString( textBeforeCursor ) > 
            mWide / 2 - mBorderWide
            &&
            mFont->measureString( textAfterCursor ) > 
            mWide / 2 - mBorderWide ) {

            // trim both ends

            while( mFont->measureString( textBeforeCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                }
        
            while( mFont->measureString( textAfterCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ wcslen( textAfterCursor ) - 1 ] = '\0';
                }
            }
        else if( mFont->measureString( textBeforeCursor ) > 
                 mWide / 2 - mBorderWide ) {

            // just trim front
            unicode *sumText = concatonate( textBeforeCursor, textAfterCursor );
            
            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }    
        else if( mFont->measureString( textAfterCursor ) > 
                 mWide / 2 - mBorderWide ) {
            
            // just trim back
            unicode *sumText = concatonate( textBeforeCursor, textAfterCursor );

            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ strlen( textAfterCursor ) - 1 ] = '\0';
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }
        }

    
    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }
    
    mDrawnText = concatonate( textBeforeCursor, textAfterCursor );

    char leftAlign = true;
    char cursorCentered = false;
    doublePair centerPos = { 0, 0 };
    
    if( ! tooLongFront ) {
        mFont->drawString( mDrawnText, textPos, alignLeft );
        mDrawnTextX = textPos.x;
        }
    else if( tooLongFront && ! tooLongBack ) {
        
        leftAlign = false;

        doublePair textPos2 = { mWide/2 - mBorderWide, 0 };

        mFont->drawString( mDrawnText, textPos2, alignRight );
        mDrawnTextX = textPos2.x - mFont->measureString( mDrawnText );
        }
    else {
        // text around perfectly centered cursor
        cursorCentered = true;
        
        double beforeLength = mFont->measureString( textBeforeCursor );
        
        double xDiff = centerPos.x - ( textPos.x + beforeLength );
        
        doublePair textPos2 = textPos;
        textPos2.x += xDiff;

        mFont->drawString( mDrawnText, textPos2, alignLeft );
        mDrawnTextX = textPos2.x;
        }
		
		
	if ( mFocused ) {
		
		if( mUseClearButton && mText[0] != 0 ) {
			float pixWidth = mCharWidth / 8;
			float buttonWidth = mFont->measureString( "x" ) + pixWidth * 2;
			float buttonRightOffset = buttonWidth / 2 + pixWidth * 2;
			doublePair lineDeleteButtonPos = { mWide / 2 - buttonRightOffset, 0 };
			if ( onClearButton ) {
				setDrawColor( 0, 0, 0, 0.5 );
				drawRect( 
					lineDeleteButtonPos.x - buttonRightOffset / 2 - mBorderWide / 2, 
					lineDeleteButtonPos.y - buttonRightOffset / 2 - mBorderWide / 2, 
					lineDeleteButtonPos.x + buttonRightOffset / 2 + mBorderWide / 2, 
					lineDeleteButtonPos.y + buttonRightOffset / 2 + mBorderWide / 2 );
			}
			setDrawColor( 1, 1, 1, 1 );
			oldMainFont->drawString( "x", lineDeleteButtonPos, alignCenter );
		}
		
		if( mRawText[0] != 0 ) {
			int numLines;
			unicode **lines = split( mRawText, L"\n", &numLines );
			
			for( int i=0; i<numLines; i++ ) {
				
				doublePair linePos = { centerPos.x, centerPos.y - (i + 1) * mHigh };
				float backgroundAlpha = 0.3;
				if( hoverIndex == i ) backgroundAlpha = 0.7;
				setDrawColor( 0, 0, 0, 1 );
				drawRect( - mWide / 2, linePos.y - mHigh / 2, 
					mWide / 2, linePos.y + mHigh / 2 );
				setDrawColor( 1, 1, 1, backgroundAlpha );
				drawRect( - mWide / 2, linePos.y - mHigh / 2, 
					mWide / 2, linePos.y + mHigh / 2 );
				doublePair lineTextPos = { textPos.x, textPos.y - (i + 1) * mHigh };
					
				setDrawColor( 0, 0, 0, 0.5 );
				float buttonRightOffset = oldMainFont->measureString( "x" );
				doublePair lineDeleteButtonPos = { mWide / 2 - buttonRightOffset, lineTextPos.y };
				if( hoverIndex == i && nearRightEdge ) {
					drawRect( 
						lineDeleteButtonPos.x - buttonRightOffset / 2 - mBorderWide / 2, 
						lineDeleteButtonPos.y - buttonRightOffset / 2 - mBorderWide / 2, 
						lineDeleteButtonPos.x + buttonRightOffset / 2 + mBorderWide / 2, 
						lineDeleteButtonPos.y + buttonRightOffset / 2 + mBorderWide / 2 );
					}
				
				setDrawColor( 1, 1, 1, 1 );
				char *lineText = stringDuplicate( lines[i] );
				
				if( mFont->measureString( lineText ) 
					+ mFont->measureString( "...   " ) 
					> mWide - 2 * mBorderWide ) {

					while( mFont->measureString( lineText ) + mFont->measureString( "...   " ) > 
						   mWide - 2 * mBorderWide ) {
						
						lineText[ strlen( lineText ) - 1 ] = '\0';
						
						}
						
					lineText = concatonate( lineText, L"...   " );
					
					}
				
				mFont->drawString( lineText, lineTextPos, alignLeft );
				
				setDrawColor( 1, 1, 1, 1 );
				oldMainFont->drawString( "x", lineDeleteButtonPos, alignCenter );
				
				
				
				delete [] lines[i];
				}
			delete [] lines;
			
			}
		}
	

    double shadeWidth = 4 * mCharWidth;
    
    if( shadeWidth > middleWidth / 2 ) {
        shadeWidth = middleWidth / 2;
        }

    if( tooLongFront ) {
        // draw shaded overlay over left of string
        
        double verts[] = { rectStartX, rectStartY,
                           rectStartX, rectEndY,
                           rectStartX + shadeWidth, rectEndY,
                           rectStartX + shadeWidth, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0 };

        drawQuads( 1, verts , vertColors );
        }
    if( tooLongBack ) {
        // draw shaded overlay over right of string
        
        double verts[] = { rectEndX - shadeWidth, rectStartY,
                           rectEndX - shadeWidth, rectEndY,
                           rectEndX, rectEndY,
                           rectEndX, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1 };

        drawQuads( 1, verts , vertColors );
        }
    
    if( mFocused && mCursorDrawPosition > -1 ) {            
        // make measurement to draw cursor

        unicode *beforeCursorText = stringDuplicate( mDrawnText );
        
        beforeCursorText[ mCursorDrawPosition ] = '\0';
        
        
        double cursorXOffset;

        if( cursorCentered ) {
            cursorXOffset = mWide / 2 - mBorderWide;
            }
        else if( leftAlign ) {
            cursorXOffset = mFont->measureString( textBeforeCursor );
            if( cursorXOffset == 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        else {
            double afterLength = mFont->measureString( textAfterCursor );
            cursorXOffset = ( mWide - 2 * mBorderWide ) - afterLength;

            if( afterLength > 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        

        
        delete [] beforeCursorText;
        
        setDrawColor( 0, 0, 0, 0.5 );
        
        drawRect( textPos.x + cursorXOffset, 
                  rectStartY - pixWidth,
                  textPos.x + cursorXOffset + pixWidth, 
                  rectEndY + pixWidth );
        }
    
    
    if( ! mActive ) {
        setDrawColor( 0, 0, 0, 0.5 );
        // dark overlay
        drawRect( - mWide / 2, - mHigh / 2, 
                  mWide / 2, mHigh / 2 );
        }
        

    delete [] textBeforeCursorBase;
    delete [] textAfterCursorBase;
    }
	
	
int DropdownList::insideIndex( float inX, float inY ) {
	if( !mFocused ) return -1;
	if( fabs( inX ) >= mWide / 2 ) return -1;
	int index = - ( inY - mHigh / 2 ) / mHigh;
	if( index <= 0 ) return -1;
	index = index - 1;
	if( index >= listLen ) return -1;
	return index;
    }
	
char DropdownList::isInsideTextBox( float inX, float inY ) {
    return fabs( inX ) < mWide / 2 &&
        fabs( inY ) < mHigh / 2;
    }
	
char DropdownList::isNearRightEdge( float inX, float inY ) {
	float pixWidth = mCharWidth / 8;
	float buttonWidth = mFont->measureString( "x" ) + pixWidth * 2;
	float buttonRightOffset = buttonWidth / 2 + pixWidth * 2;
    return inX > 0 && 
		fabs( inX - ( mWide / 2 - buttonRightOffset ) ) < buttonWidth / 2;
    }



void DropdownList::pointerMove( float inX, float inY ) {
	hoverIndex = insideIndex( inX, inY );
	nearRightEdge = isNearRightEdge( inX, inY );
	onClearButton = isInsideTextBox( inX, inY ) && nearRightEdge;
	mHover = hoverIndex != -1 || isInsideTextBox( inX, inY );
    }


void DropdownList::pointerDown( float inX, float inY ) {
    
	int mouseButton = getLastMouseButton();
	if ( mouseButton == MouseButton::WHEELUP || mouseButton == MouseButton::WHEELDOWN ) { return; }
    
	hoverIndex = insideIndex( inX, inY );
	//if( !isInsideTextBox( inX, inY ) && !nearRightEdge ) unfocus();
	if( !isInsideTextBox( inX, inY ) && !(nearRightEdge && hoverIndex != -1) ) unfocus();
	if( onClearButton && mFocused ) setText( "" );
	if( hoverIndex == -1 ) return;
	if( !nearRightEdge ) {
		selectOption( hoverIndex );
	} else {
		deleteOption( hoverIndex );
		}
    fireActionPerformed( this );
    }


void DropdownList::pointerUp( float inX, float inY ) {
    if( !isInsideTextBox( inX, inY ) && !nearRightEdge ) unfocus();
    
    if( mIgnoreMouse || mIgnoreEvents ) {
        return;
        }
        
	int mouseButton = getLastMouseButton();
	if ( mouseButton == MouseButton::WHEELUP || mouseButton == MouseButton::WHEELDOWN ) { return; }
    
    if( inX > - mWide / 2 &&
        inX < + mWide / 2 &&
        inY > - mHigh / 2 &&
        inY < + mHigh / 2 ) {

        char wasHidden = mContentsHidden;

        focus();

        if( wasHidden ) {
            // don't adjust cursor from where it was
            }
        else {
            
            int bestCursorDrawPosition = mCursorDrawPosition;
            double bestDistance = mWide * 2;
            
            int drawnTextLength = strlen( mDrawnText );
            
            // find gap between drawn letters that is closest to clicked x
            
            for( int i=0; i<=drawnTextLength; i++ ) {
                
                unicode *textCopy = stringDuplicate( mDrawnText );
                
                textCopy[i] = '\0';
                
                double thisGapX = 
                    mDrawnTextX + 
                    mFont->measureString( textCopy ) +
                    mFont->getCharSpacing() / 2;
                
                delete [] textCopy;
                
                double thisDistance = fabs( thisGapX - inX );
                
                if( thisDistance < bestDistance ) {
                    bestCursorDrawPosition = i;
                    bestDistance = thisDistance;
                    }
                }
            
            int cursorDelta = bestCursorDrawPosition - mCursorDrawPosition;
            
            mCursorPosition += cursorDelta;
            }
        }
    }




unsigned unicode DropdownList::processCharacter( unsigned unicode inASCII ) {

    unsigned unicode processedChar = inASCII;
        
    if( mForceCaps ) {
        processedChar = towupper( inASCII );
    }

    if( mForbiddenChars != NULL ) {
        int num = wcslen( mForbiddenChars );
            
        for( int i=0; i<num; i++ ) {
            if( mForbiddenChars[i] == processedChar ) {
                return 0;
                }
            }
        }
        

    if( mAllowedChars != NULL ) {
        int num = wcslen( mAllowedChars );
            
        char allowed = false;
            
        for( int i=0; i<num; i++ ) {
            if( mAllowedChars[i] == processedChar ) {
                allowed = true;
                break;
                }
            }

        if( !allowed ) {
            return 0;
            }
        }
    else {
        // no allowed list specified 
        
        if( processedChar == (unicode)'\r' ) {
            // \r only permitted if it is listed explicitly
            return 0;
            }
        }
        

    return processedChar;
    }



void DropdownList::insertCharacter( unicode inASCII ) {
    
    if( isAnythingSelected() ) {
        // delete selected first
        deleteHit();
        }

    // add to it
    unicode *oldText = mText;
    int length = wcslen(oldText);
    if( mMaxLength != -1 &&
        length >= (unsigned int) mMaxLength ) {
        // max length hit, don't add it
        return;
        }
    
    unicode* text = new unicode[length+1];
    memcpy(text, mText, sizeof(unicode)*(mCursorPosition-1));
    text[mCursorPosition-1] = inASCII;
    wcscpy(text+mCursorPosition, mText+mCursorPosition)
    
    delete []mText;
    
    mText = text;
    mTextLen = wcslen( mText );
    mCursorPosition++;
}



void DropdownList::insertString( unicode *inString ) {
    if( isAnythingSelected() ) {
        // delete selected first
        deleteHit();
        }
    
    // add to it
    unicode *oldText = mText;
    
   // add to it
    unicode *oldText = mText;
    int length = wcslen(oldText);
    int inlen = wcslen(inString);
    if( mMaxLength != -1 &&
        (length-inlen+1) >= (unsigned int) mMaxLength ) {
        // max length hit, don't add it
        return;
    }
    
    
    unicode* text = new unicode[length+inlen];
    memcpy(text, mText, sizeof(unicode)*(mCursorPosition-1));
    wcscpy(mText+mCursorPosition-1, inString);
    wcscpy(text+mCursorPosition-1+inlen, mText+mCursorPosition)
    
    delete []mText;
    
    mText = text;
    mTextLen = wcslen( mText );
    mCursorPosition+=inlen;
}



int DropdownList::getCursorPosition() {
    return mCursorPosition;
    }


void DropdownList::cursorReset() {
    mCursorPosition = 0;
    }



void DropdownList::setIgnoreArrowKeys( char inIgnore ) {
    mIgnoreArrowKeys = inIgnore;
    }



void DropdownList::setIgnoreMouse( char inIgnore ) {
    mIgnoreMouse = inIgnore;
    }



double DropdownList::getRightEdgeX() {
    
    return mX + mWide / 2;
    }



double DropdownList::getLeftEdgeX() {
    
    return mX - mWide / 2;
    }



double DropdownList::getWidth() {
    
    return mWide;
    }



void DropdownList::setWidth( double inWide ) {
    
    mWide = inWide;
    }



void DropdownList::setFireOnAnyTextChange( char inFireOnAny ) {
    mFireOnAnyChange = inFireOnAny;
    }


void DropdownList::setFireOnLoseFocus( char inFireOnLeave ) {
    mFireOnLeave = inFireOnLeave;
    }




void DropdownList::keyDown( unicode inASCII ) {
    if( !mFocused ) {
        return;
    }
    mCursorFlashSteps = 0;
    
    if( isCommandKeyDown() ) {
        // not a normal key stroke (command key)
        // ignore it as input

        if( mUsePasteShortcut && ( inASCII == (unicode)'v' || inASCII == 22 ) ) {
            // ctrl-v is SYN on some platforms
            
            // paste!
            if( isClipboardSupported() ) {
                char *clipboardText = getClipboardText();
                int len = strlen( clipboardText );
                unicode* text = new unicode[len+1];
                utf8ToUnicode(clipboardText, text);

                for( int i=0; i<len; i++ ) {
                    
                    unicode processedChar = 
                        processCharacter( text[i] );    

                    if( processedChar != 0 ) {
                        insertCharacter( processedChar );
                    }
                }
                delete [] clipboardText;
                delete [] text;

                mHoldDeleteSteps = -1;
                mFirstDeleteRepeatDone = false;
                
                clearArrowRepeat();
                
                if( mFireOnAnyChange ) {
                    fireActionPerformed( this );
                    }
                }
            }
			
        if( mUsePasteShortcut && inASCII + 64 == (unicode)toupper('c') )  {
			char *text = getText();
			setClipboardText( text );
			delete [] text;
		}

        // but ONLY if it's an alphabetical key (A-Z,a-z)
        // Some international keyboards use ALT to type certain symbols

        if( ( inASCII >= (unicode)'A' && inASCII <= (unicode)'Z' )
            ||
            ( inASCII >= (unicode)'a' && inASCII <= (unicode)'z' ) ) {
            
            return;
        }
        
    }
    

    if( inASCII == 127 || inASCII == 8 ) {
        // delete
        deleteHit();
        
        mHoldDeleteSteps = 0;

        clearArrowRepeat();
    }
    else if( inASCII == 13 ) {
        // enter hit in field
        unicode processedChar = processCharacter( unicode );    

        if( processedChar != 0 ) {
            // newline is allowed
            insertCharacter( processedChar );
            
            mHoldDeleteSteps = -1;
            mFirstDeleteRepeatDone = false;
            
            clearArrowRepeat();
            
            if( mFireOnAnyChange ) {
                fireActionPerformed( this );
            }
        }
        else {
            // newline not allowed in this field
            fireActionPerformed( this );
        }
    }
    else if( inASCII >= 32 ) {

        unicode processedChar = processCharacter( inASCII );    

        if( processedChar != 0 ) {
            insertCharacter( processedChar );
        }
        
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;

        clearArrowRepeat();

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
        }
    }    
}



void DropdownList::keyUp( unicode inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // end delete hold down
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;
    }
}



void DropdownList::deleteHit() {
    if( mCursorPosition > 0 || isAnythingSelected() ) {
        mCursorFlashSteps = 0;
    
        int newCursorPos = mCursorPosition - 1;


        if( isAnythingSelected() ) {
            // selection delete
            
            mCursorPosition = mSelectionEnd;
            
            newCursorPos = mSelectionStart;

            mSelectionStart = -1;
            mSelectionEnd = -1;
        }
        else if( isCommandKeyDown() ) {
            // word delete 

            newCursorPos = mCursorPosition;

            // skip non-space, non-newline characters
            while( newCursorPos > 0 &&
                   mText[ newCursorPos - 1 ] != (unicode)' ' &&
                   mText[ newCursorPos - 1 ] != (unicode)'\r' ) {
                newCursorPos --;
            }
        
            // skip space and newline characters
            while( newCursorPos > 0 &&
                   ( mText[ newCursorPos - 1 ] == (unicode)' ' ||
                     mText[ newCursorPos - 1 ] == (unicode)'\r' ) ) {
                newCursorPos --;
            }
        }
        
        // section cleared no matter what when delete is hit
        mSelectionStart = -1;
        mSelectionEnd = -1;


        unicode *oldText = mText;
        
        unicode *preCursor = stringDuplicate( mText );
        preCursor[ newCursorPos ] = '\0';
        unicode *postCursor = &( mText[ mCursorPosition ] );

        mText = concatonate(preCursor, postCursor);
        mTextLen = wcslen( mText );
        
        delete [] preCursor;

        delete [] oldText;

        mCursorPosition = newCursorPos;

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
        }
    }
}



void DropdownList::clearArrowRepeat() {
    for( int i=0; i<2; i++ ) {
        mHoldArrowSteps[i] = -1;
        mFirstArrowRepeatDone[i] = false;
    }
}



void DropdownList::leftHit() {
    mCursorFlashSteps = 0;
    
    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionStart;
        }
        else {
            mCursorPosition = *mSelectionAdjusting;
        }
    }

    if( ! isShiftKeyDown() ) {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionStart + 1;
        }

        mSelectionStart = -1;
        mSelectionEnd = -1;
    }

    if( isCommandKeyDown() ) {
        // word jump 

        // skip non-space, non-newline characters
        while( mCursorPosition > 0 &&
               mText[ mCursorPosition - 1 ] != (unicode)' ' &&
               mText[ mCursorPosition - 1 ] != (unicode)'\r' ) {
            mCursorPosition --;
        }
        
        // skip space and newline characters
        while( mCursorPosition > 0 &&
               ( mText[ mCursorPosition - 1 ] == (unicode)' ' ||
                 mText[ mCursorPosition - 1 ] == (unicode)'\r' ) ) {
            mCursorPosition --;
        }
        
    }
    else {    
        mCursorPosition --;
        if( mCursorPosition < 0 ) {
            mCursorPosition = 0;
        }
    }

    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
    }

}



void DropdownList::rightHit() {
    mCursorFlashSteps = 0;
    
    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionEnd;
        }
        else {
            mCursorPosition = *mSelectionAdjusting;
        }
    }
    
    if( ! isShiftKeyDown() ) {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionEnd - 1;
        }
            
        mSelectionStart = -1;
        mSelectionEnd = -1;
    }

    if( isCommandKeyDown() ) {
        // word jump 
        int textLen = wcslen( mText );
        
        // skip space and newline characters
        while( mCursorPosition < textLen &&
               ( mText[ mCursorPosition ] == (unicode)' ' ||
                 mText[ mCursorPosition ] == (unicode)'\r'  ) ) {
            mCursorPosition ++;
        }

        // skip non-space and non-newline characters
        while( mCursorPosition < textLen &&
               mText[ mCursorPosition ] != (unicode)' ' &&
               mText[ mCursorPosition ] != (unicode)'\r' ) {
            mCursorPosition ++;
        }
        
        
    }
    else {
        mCursorPosition ++;
        if( mCursorPosition > (int)strlen( mText ) ) {
            mCursorPosition = strlen( mText );
        }
    }

    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
    }
    
}




void DropdownList::specialKeyDown( int inKeyCode ) {
    if( !mFocused ) {
        return;
    }
    
    mCursorFlashSteps = 0;
    
    switch( inKeyCode ) {
        case MG_KEY_LEFT:
            if( ! mIgnoreArrowKeys ) {    
                leftHit();
                clearArrowRepeat();
                mHoldArrowSteps[0] = 0;
            }
            break;
        case MG_KEY_RIGHT:
            if( ! mIgnoreArrowKeys ) {
                rightHit(); 
                clearArrowRepeat();
                mHoldArrowSteps[1] = 0;
            }
            break;
        default:
            break;
    }
    
}



void DropdownList::specialKeyUp( int inKeyCode ) {
    if( inKeyCode == MG_KEY_LEFT ) {
        mHoldArrowSteps[0] = -1;
        mFirstArrowRepeatDone[0] = false;
    }
    else if( inKeyCode == MG_KEY_RIGHT ) {
        mHoldArrowSteps[1] = -1;
        mFirstArrowRepeatDone[1] = false;
    }
}



void DropdownList::focus() {
    
    if( sFocusedDropdownList != NULL && sFocusedDropdownList != this ) {
        // unfocus last focused
        sFocusedDropdownList->unfocus();
    }
		
    TextField::unfocusAll();

    mFocused = true;
    sFocusedDropdownList = this;

    mContentsHidden = false;
}



void DropdownList::unfocus() {
    mFocused = false;
 
    // hold-down broken if not focused
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();

    if( sFocusedDropdownList == this ) {
        sFocusedDropdownList = NULL;
        if( mFireOnLeave ) {
            fireActionPerformed( this );
        }
    }    
}



char DropdownList::isFocused() {
    return mFocused;
}



void DropdownList::setDeleteRepeatDelays( int inFirstDelaySteps,
                                       int inNextDelaySteps ) {
    sDeleteFirstDelaySteps = inFirstDelaySteps;
    sDeleteNextDelaySteps = inNextDelaySteps;
}



char DropdownList::isAnyFocused() {
    if( sFocusedDropdownList != NULL ) {
        return true;
    }
    return false;
}


        
void DropdownList::unfocusAll() {
    
    if( sFocusedDropdownList != NULL ) {
        // unfocus last focused
        sFocusedDropdownList->unfocus();
    }

    sFocusedDropdownList = NULL;
}




void DropdownList::setLabelSide( char inLabelOnRight ) {
    mLabelOnRight = inLabelOnRight;
}



void DropdownList::setLabelTop( char inLabelOnTop ) {
    mLabelOnTop = inLabelOnTop;
}


        
char DropdownList::isAnythingSelected() {
    return 
        ( mSelectionStart != -1 && 
          mSelectionEnd != -1 &&
          mSelectionStart != mSelectionEnd );
}



unicode *DropdownList::getSelectedText() {

    if( ! isAnythingSelected() ) {
        return NULL;
    }
    
    unicode *textCopy = stringDuplicate( mText );

    textCopy[ mSelectionEnd ] = '\0';
    
    unicode *startPointer = &( textCopy[ mSelectionStart ] );  
    unicode *returnVal = stringDuplicate( startPointer );

    delete [] textCopy;

    return returnVal;
}



void DropdownList::fixSelectionStartEnd() {
    if( mSelectionEnd < mSelectionStart ) {
        int temp = mSelectionEnd;
        mSelectionEnd = mSelectionStart;
        mSelectionStart = temp;

        if( mSelectionAdjusting == &mSelectionStart ) {
            mSelectionAdjusting = &mSelectionEnd;
        }
        else if( mSelectionAdjusting == &mSelectionEnd ) {
            mSelectionAdjusting = &mSelectionStart;
        }
    }
    else if( mSelectionEnd == mSelectionStart ) {
        mSelectionAdjusting = &mSelectionEnd;
    }
    
}



void DropdownList::setShiftArrowsCanSelect( char inCanSelect ) {
    mShiftPlusArrowsCanSelect = inCanSelect;
}



void DropdownList::usePasteShortcut( char inShortcutOn ) {
    mUsePasteShortcut = inShortcutOn;
}
	
	
void DropdownList::useClearButton( char inClearButtonOn ) {
    mUseClearButton = inClearButtonOn;
 }


char DropdownList::isMouseOver() {
    return mHover;
}