package com.example.testapplication.CustomViews;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;

import androidx.annotation.Nullable;


public class DrawSquares extends View
{
    private Rect mSquare;
    private Paint mPaintSquare;

    public String mName;
    public int mX = 0;
    public int mY = 0;
    public int mViewId;


    public DrawSquares(Context context)
    {
        super(context);
        init(null);
    }

    public DrawSquares(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        init(attrs);
    }

    public DrawSquares(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
        init(attrs);
    }

    public DrawSquares(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        init(attrs);

    }

    private void init(@Nullable AttributeSet set)
    {

        mSquare = new Rect();
        mPaintSquare = new Paint(Paint.ANTI_ALIAS_FLAG);
    }

    @Override
    protected void onDraw(Canvas canvas)
    {
        mSquare.left = mX;
        mSquare.top = mY;
        mSquare.right = mSquare.left + dpToPixels(5);
        mSquare.bottom = mSquare.top + dpToPixels(5);

        mPaintSquare.setColor(Color.GREEN);
        canvas.drawRect(mSquare, mPaintSquare);
    }

    public int dpToPixels(int dp)
    {
        int pixels;

        pixels = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());

        return pixels;
    }

}

