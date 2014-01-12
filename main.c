#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>
#include<GL/gl.h>
#include<GL/glu.h>
#include<GL/glut.h>

#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)
#define MAX 100
#define INF 1<<20
#define EPS 0.001
#define PROJECTOR 1
#define MIRROR 2
#define BLOCK 3

struct point
{
    float x;
    float y;
};
typedef struct point point;

struct line
{
    point p1;
    point p2;
    float m;
    float c;
};
typedef struct line line;

struct world
{
    float width;
    float height;
    point p[4];
    line l[4];
} world;

struct Mirror
{
    line l;
};
typedef struct Mirror Mirror;

struct Block
{
    line l;
};
typedef struct Block Block;

struct Projector
{
    line l;
    int num_pixels;
    float distance;
    point d;
    line pixels[MAX];
};
typedef struct Projector Projector;

struct colors
{
    float r;
    float g;
    float b;
};
typedef struct colors colors;

struct Mouse
{
    point p;
} mouse;
typedef struct Mouse Mouse;

int count = 0,w,h;
int num_mirrors = 0;
int num_blocks = 0;
int num_projectors = 0;
int num_world = 4;
float world_z = 0;
int select_number = 0;
int select_type = PROJECTOR;
float trans_val = 0.5;
float rotat_val = 0.1;
int random_walk_mode = 0;
int light_transport_mode = 0;
int gaze_cursor_mode = 0;
int insert_object_mode = 0;
int insert_object_type;
int WindowHeight;
int WindowWidth;
int update_timer = 10;
Block block[MAX];
Mirror mirror[MAX];
Projector projector[MAX];
colors color[MAX*10];

double rand_num()
{
    return (double)rand() / (double)RAND_MAX ;
}

float distance_between_points(point p1,point p2)
{
    float dist = (p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y);
    return sqrt(dist);
}

float find_slope(point p1,point p2)
{
    if(p2.x == p1.x)
        return INF;
    float slope = (p2.y - p1.y) / (p2.x - p1.x);
    return slope;
}

line find_slope_intercept(line l)
{
    l.m = find_slope(l.p1,l.p2);
    if(l.m == INF)
        l.c = INF;
    else
        l.c = l.p1.y - (l.m)*(l.p1.x);
    return l;
}

void divide_line(point p1,point p2,int num_points ,int projector_num)
{
    num_points -= 2;
    projector[projector_num].pixels[0].p1 = p1;
    projector[projector_num].pixels[0].p2 = projector[projector_num].d;
    projector[projector_num].pixels[num_points+1].p1 = p2;
    projector[projector_num].pixels[num_points+1].p2 = projector[projector_num].d;
    projector[projector_num].pixels[0] = find_slope_intercept(projector[projector_num].pixels[0]);
    projector[projector_num].pixels[num_points+1] = find_slope_intercept(projector[projector_num].pixels[num_points+1]);
    int i;
    for(i=1; i<=num_points; i++)
    {
        projector[projector_num].pixels[i].p1.x = ( ((num_points - i + 1)*p1.x) + (i*p2.x) ) / (num_points+1);
        projector[projector_num].pixels[i].p1.y = ( ((num_points - i + 1)*p1.y) + (i*p2.y) ) / (num_points+1);
        projector[projector_num].pixels[i].p2 = projector[projector_num].d;
        projector[projector_num].pixels[i] = find_slope_intercept(projector[projector_num].pixels[i]);
    }
}

point find_intersection(line l1,line l2)
{
    point p;
    if(l1.m == INF)
    {
        p.x = l1.p1.x;
        p.y = l2.m*p.x + l2.c;
        return p;
    }
    if(l2.m == INF)
    {
        p.x = l2.p2.x;
        p.y = l1.m*p.x + l1.c;
        return p;
    }
    p.x = (l2.c-l1.c)/(l1.m-l2.m);
    p.y = l1.m*p.x + l1.c;
    return p;
}

int find_side(point p1,point p2,point p3)
{
    float check = (p2.x - p1.x)*(p3.y - p1.y) - (p2.y - p1.y)*(p3.x - p1.x);
    if(check == 0)
        return 0;
    else if(check > 0)
        return -1; //Left
    else
        return 1;  //Right
}

point find_source_point(Projector p)
{
    point q1,q2;
    float x3 = (p.l.p1.x + p.l.p2.x) / 2;
    float y3 = (p.l.p1.y + p.l.p2.y) / 2;
    if(p.l.m == 0)
    {
        q1.x = x3;
        q1.y = y3 - p.distance;
        q2.x = x3;
        q2.y = y3 + p.distance;
        if(find_side(p.l.p1,p.l.p2,q1) == 1)
            return q1;
        else
            return q2;
    }
    float m1 = -1/p.l.m;
    float c1 = y3 - m1*x3;
    float d = p.distance;
    float s = sqrt( (d*d) / (m1*m1+1) );
    q1.x = x3 - s;
    q1.y = m1*q1.x+c1;
    q2.x = x3 + s;
    q2.y = m1*q2.x+c1;
    if(find_side(p.l.p1,p.l.p2,q1) == 1)
        return q1;
    else
        return q2;
}

float find_angle(line l1,line l2)
{
    if(l1.m == INF)
        return abs(PI/2 - atan(l2.m));
    if(l2.m == INF)
        return abs(PI/2 - atan(l1.m));
    if(abs(l1.m - l2.m) < EPS)
        return 0;
    float num = l1.m - l2.m;
    float den = 1 + l1.m*l2.m;

    if(abs(den) < EPS)
        return atan(PI/2);
    return abs(atan(num/den));
}

float find_angle2(point p1,point p2,point p,point q)
{
    float x1 = p2.x - p1.x;
    float y1 = p2.y - p1.y;
    float x2 = p.x - q.x;
    float y2 = p.y - q.y;
    float num = x1*x2 + y1*y2;
    float den = (sqrt(x1*x1 + y1*y1))*(sqrt(x2*x2+y2*y2));
    float s = acos(num/den);
    return s > PI/2 ? PI - s:s;
}

int check_point_on_line(line l,point p)
{
    if(p.y == p.x*l.m + l.c)
        return 1;
    else
        return 0;
}

float distance_line_point(line l,point p)
{
    float px = l.p2.x - l.p1.x;
    float py = l.p2.y - l.p1.y;
    float len = px*px + py*py;
    float u = ((p.x - l.p1.x)*px + (p.y - l.p1.y)*py)/len;
    if(u>1.0)
        u = 1.0;
    if(u<0)
        u = 0;
    float dx = l.p1.x + u*px - p.x;
    float dy = l.p1.y + u*py - p.y;
    return sqrt(dx*dx + dy*dy);
}

/*int check_point_on_line_segment(line l,point p)
{
    if(l.p1.x > l.p2.x)
    {
        if(l.p1.y > l.p2.y)
        {
            if(p.x >= l.p2.x && p.x <= l.p1.x && p.y >= l.p2.y && p.y <= l.p1.y)
                return 1;
            else
                return 0;
        }
        else
        {
            if(p.x >= l.p2.x && p.x <= l.p1.x && p.y >= l.p1.y && p.y <= l.p2.y)
                return 1;
            else
                return 0;
        }
    }
    else
    {
        if(l.p1.y > l.p2.y)
        {
            if(p.x <= l.p2.x && p.x >= l.p1.x && p.y >= l.p2.y && p.y <= l.p1.y)
                return 1;
            else
                return 0;
        }
        else
        {
            if(p.x <= l.p2.x && p.x >= l.p1.x && p.y >= l.p1.y && p.y <= l.p2.y)
                return 1;
            else
                return 0;
        }
    }
}*/

int check_point_on_line_segment(line l,point p)
{
    float x = abs(distance_between_points(l.p1,p) + distance_between_points(p,l.p2) - distance_between_points(l.p1,l.p2));
    return x < EPS;
}

void draw_point(point p)
{
    /*glBegin(GL_POINTS);
    glColor3f(rand_num(),rand_num(),rand_num());
    glVertex2f(p.x,p.y);
    glEnd();*/
    int i;
    float rad = 1;
    glColor3f(rand_num(),rand_num(),rand_num());
    glBegin(GL_TRIANGLE_FAN);
    for(i=0 ; i<360 ; i++)
        glVertex2f(p.x + rad * cos(DEG2RAD(i)), p.y + rad * sin(DEG2RAD(i)));
    glEnd();
}

void draw_line(line l)
{
    glBegin(GL_LINES);
    glVertex2f(l.p1.x,l.p1.y);
    glVertex2f(l.p2.x,l.p2.y);
    glEnd();
}

void draw_selected_line(line l)
{
    glEnable(GL_LINE_STIPPLE);
    glLineWidth(8);
    glLineStipple(1,0x00FF);
    glColor3f(rand_num(),rand_num(),rand_num());
    glBegin(GL_LINES);
    glVertex2f(l.p1.x,l.p1.y);
    glVertex2f(l.p2.x,l.p2.y);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
    glLineWidth(2);
}

void draw_line2(point p1,point p2)
{
    if(light_transport_mode)
    {
        int i = 0;
        float dist = distance_between_points(p1,p2);
        float d0 = dist/8;
        float d1 = rand_num()*d0;
        float theta = atan2((p2.y-p1.y),(p2.x-p1.x));
        float tot_dist = d1;
        float ctheta = cos(theta),stheta = sin(theta);
        p2.x = p1.x + d1*ctheta;
        p2.y = p1.y + d1*stheta;
        glBegin(GL_LINES);
        glVertex2f(p1.x,p1.y);
        glVertex2f(p2.x,p2.y);
        while(1)
        {
            p1 = p2;
            if(tot_dist+d0 > dist)
            {
                d0 = dist - tot_dist;
                p2.x = p1.x + d0*ctheta;
                p2.y = p1.y + d0*stheta;
                if(i%2)
                {
                    glVertex2f(p1.x,p1.y);
                    glVertex2f(p2.x,p2.y);
                }
                break;
            }
            p2.x = p1.x + d0*ctheta;
            p2.y = p1.y + d0*stheta;
            if(i%2)
            {
                glVertex2f(p1.x,p1.y);
                glVertex2f(p2.x,p2.y);
            }
            tot_dist += d0;
            i++;
        }
        glEnd();
        return;
    }
    glBegin(GL_LINES);
    glVertex2f(p1.x,p1.y);
    glVertex2f(p2.x,p2.y);
    glEnd();
}

void draw_box(float width,float height)
{
    glBegin(GL_LINE_LOOP);
    glColor3f(0,0,0);
    glVertex2f(-width/2,-height/2);
    glVertex2f(width/2,-height/2);
    glVertex2f(width/2,height/2);
    glVertex2f(-width/2,height/2);
    glEnd();
}

int find_side2(point p,point q)
{
    if(p.x > q.x)
        return 1;
    else
        return -1;
}

void draw_reflections(line l,int m,int side)
{
    int i,j,k;
    point p,fp;
    float dist = INF;
    int type = 1,num;
    for(k=0; k<num_mirrors; k++)
    {
        if(k!=m)
        {
            p = find_intersection(mirror[k].l,l);
            if( (abs(p.x) <= world.width/2) && (abs(p.y) <= world.height/2) && check_point_on_line_segment(mirror[k].l,p) )
                if(find_side(mirror[m].l.p1,mirror[m].l.p2,p) == side)
                {
                    float d = distance_between_points(p,l.p1);
                    if( d < dist )
                    {
                        fp = p;
                        dist = d;
                        type = 1;
                        num = k;
                        if(find_side(mirror[k].l.p1,mirror[k].l.p2,l.p1) == -1)
                            type = 2;
                    }
                }
        }
    }
    for(k=0; k<num_blocks; k++)
    {
        p = find_intersection(block[k].l,l);
        if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(block[k].l,p))
            if(find_side(mirror[m].l.p1,mirror[m].l.p2,p) == side)
            {
                float d = distance_between_points(p,l.p1);
                if( d < dist )
                {
                    fp = p;
                    dist = d;
                    type = 2;
                }
            }
    }
    for(k=0; k<num_world; k++)
    {
        p = find_intersection(world.l[k],l);
        if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(world.l[k],p))
            if(find_side(mirror[m].l.p1,mirror[m].l.p2,p) == side)
            {
                float d = distance_between_points(p,l.p1);
                if( d < dist )
                {
                    fp = p;
                    dist = d;
                    type = 2;
                }
            }
    }
    for(k=0; k<num_projectors; k++)
    {
        p = find_intersection(projector[k].l,l);
        if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(projector[k].l,p))
            if(find_side(mirror[m].l.p1,mirror[m].l.p2,p) == side)
            {
                float d = distance_between_points(p,l.p1);
                if( d < dist )
                {
                    fp = p;
                    dist = d;
                    type = 2;
                }
            }
    }
    draw_line2(fp,l.p1);
    if(type==1)
    {
        //float angle = find_angle(l,mirror[m].l);
        line n;
        n.p1 = fp;
        n.m = tan(PI + 2*atan(mirror[num].l.m) - atan(l.m));
        n.c = n.p1.y - n.p1.x*n.m;
        draw_reflections(n,num,1);
    }
}

void draw_world()
{
    world_z = world.height+25;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -world_z);
    int i,j,k;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int c = 0;
    draw_box(world.width,world.height);
    glColor3f(color[c].r,color[c].g,color[c].b);
    for(i=0; i<num_projectors; i++)
    {
        if(select_type == PROJECTOR && select_number == i)
            draw_selected_line(projector[i].l);
        else
            draw_line(projector[i].l);
        draw_point(projector[i].d);
        glColor3f(color[c].r,color[c].g,color[c].b);
    }
    c++;
    glColor3f(color[c].r,color[c].g,color[c].b);
    for(i=0; i<num_mirrors; i++)
    {
        if(select_type == MIRROR && select_number == i)
            draw_selected_line(mirror[i].l);
        else
            draw_line(mirror[i].l);
        glColor3f(color[c].r,color[c].g,color[c].b);
    }
    c++;
    glColor3f(color[c].r,color[c].g,color[c].b);
    for(i=0; i<num_blocks; i++)
    {
        if(select_type == BLOCK && select_number == i)
            draw_selected_line(block[i].l);
        else
            draw_line(block[i].l);
        glColor3f(color[c].r,color[c].g,color[c].b);
    }
    /*for(i=0; i<num_projectors; i++)
    {
        for(j=0; j<projector[i].num_pixels; j++)
            draw_line(projector[i].pixels[j]);
        draw_point(projector[i].d);
    }*/
    for(i=0; i<num_projectors; i++)
    {
        for(j=0; j<projector[i].num_pixels; j++)
        {
            glColor3f(color[c].r,color[c].g,color[c++].b);
            point p,fp;
            float dist = INF;
            int type = 1,num;
            for(k=0; k<num_mirrors; k++)
            {
                p = find_intersection(mirror[k].l,projector[i].pixels[j]);
                if( (abs(p.x) <= world.width/2) && (abs(p.y) <= world.height/2) && check_point_on_line_segment(mirror[k].l,p) )
                    if(find_side(projector[i].l.p1,projector[i].l.p2,p) == -1)
                    {
                        float d = distance_between_points(p,projector[i].pixels[j].p1);
                        if( d < dist )
                        {
                            fp = p;
                            dist = d;
                            type = 1;
                            num = k;
                            if(find_side(mirror[k].l.p1,mirror[k].l.p2,projector[i].pixels[j].p1) == -1)
                                type = 2;
                        }
                    }
            }
            for(k=0; k<num_blocks; k++)
            {
                p = find_intersection(block[k].l,projector[i].pixels[j]);
                if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(block[k].l,p))
                    if(find_side(projector[i].l.p1,projector[i].l.p2,p) == -1)
                    {
                        float d = distance_between_points(p,projector[i].pixels[j].p1);
                        if( d < dist )
                        {
                            fp = p;
                            dist = d;
                            type = 2;
                        }
                    }
            }
            for(k=0; k<num_world; k++)
            {
                p = find_intersection(world.l[k],projector[i].pixels[j]);
                if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(world.l[k],p))
                    if(find_side(projector[i].l.p1,projector[i].l.p2,p) == -1)
                    {
                        float d = distance_between_points(p,projector[i].pixels[j].p1);
                        if( d < dist )
                        {
                            fp = p;
                            dist = d;
                            type = 2;
                        }
                    }
            }
            for(k=0; k<num_projectors; k++)
            {
                if(k!=i)
                {
                    p = find_intersection(projector[k].l,projector[i].pixels[j]);
                    if(abs(p.x) <= world.width/2 && abs(p.y) <= world.height/2 && check_point_on_line_segment(projector[k].l,p))
                        if(find_side(projector[i].l.p1,projector[i].l.p2,p) == -1)
                        {
                            float d = distance_between_points(p,projector[i].pixels[j].p1);
                            if( d < dist )
                            {
                                fp = p;
                                dist = d;
                                type = 2;
                            }
                        }
                }
            }
            draw_line2(fp,projector[i].pixels[j].p2);
            if(type==1)
            {
                //float angle = find_angle(projector[i].pixels[j],mirror[num].l);
                //float angle = find_angle2(projector[i].pixels[j].p1,fp,mirror[num].l.p1,mirror[num].l.p2);
                line n;
                n.p1 = fp;
                n.m = tan(PI + 2*atan(mirror[num].l.m) - atan(projector[i].pixels[j].m));
                n.c = n.p1.y - n.p1.x*n.m;
                draw_reflections(n,num,1);
            }
        }
    }
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glPushMatrix();
    //glTranslatef(0.0f, 0.0f, -5.0f);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glRotatef(rot,1,2,0);
    //glRasterPos2f(-0.5,0);
    //glColor4f(0.0f, 1.0f, 1.0f,1.0f);
    //glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, "Rishi Raj Singh Jhelumi");
    //glBegin(GL_POINTS);
    //glVertex3f(0,0,-world_z);
    //glEnd();
    //glPopMatrix();
    glFlush();
    //glutSwapBuffers();
}

void draw_scene(void)
{
    draw_world();
}

void light_transport(int value)
{
    draw_world();
    glutTimerFunc(update_timer,light_transport,0);
}

void reshape_window(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)w / (float)h, 0.001, 200);
    //gluOrtho2D(0, w,h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    WindowHeight = (h>1) ? h : 2;
    WindowWidth = (w>1) ? w : 2;
    glViewport(0,0, (GLsizei) w, (GLsizei) h);
    /*glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);  // Always view [0,1]x[0,1].
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();*/

}

void initRendering()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

    glEnable(GL_BLEND);
    glEnable(GL_BLEND_COLOR);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(8);
    glLineWidth(2);
    glClearColor(1,1,1,1);

    //glEnable( GL_POINT_SPRITE );
    //glActiveTexture(GL_TEXTURE0);
    //glEnable( GL_TEXTURE_2D );
    //glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    //glBindTexture(GL_TEXTURE_2D,);
    //glutFullScreen();
    //glCullFace(GL_BACK);
    /*glViewport( 0,0, w, h );
    glMatrixMode( GL_PROJECTION );
    glOrtho( 0.0, w, 0.0, h, 1.0, -1.0 );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();*/
}

void scanner()
{
    int i;
    scanf("%f%f",&world.width,&world.height);
    float w = world.width/2;
    float h = world.height/2;
    world.p[0].x = -w;
    world.p[0].y = -h;
    world.p[1].x = w;
    world.p[1].y = -h;
    world.p[2].x = w;
    world.p[2].y = h;
    world.p[3].x = -w;
    world.p[3].y = h;
    for(i=0; i<num_world; i++)
    {
        point p=world.p[i],q=world.p[(i+1)%4];
        world.l[i].p1 = p;
        world.l[i].p2 = q;
        world.l[i] = find_slope_intercept(world.l[i]);
    }

    scanf("%d",&num_projectors);
    for(i=0; i<num_projectors; i++)
    {
        scanf("%f%f%f%f",&projector[i].l.p1.x,&projector[i].l.p1.y,&projector[i].l.p2.x,&projector[i].l.p2.y);
        scanf("%f",&projector[i].distance);
        scanf("%d",&projector[i].num_pixels);
        projector[i].l = find_slope_intercept(projector[i].l);
        projector[i].d = find_source_point(projector[i]);
        divide_line(projector[i].l.p1,projector[i].l.p2,projector[i].num_pixels,i);
    }

    scanf("%d",&num_blocks);
    for(i=0; i<num_blocks; i++)
    {
        scanf("%f%f%f%f",&block[i].l.p1.x,&block[i].l.p1.y,&block[i].l.p2.x,&block[i].l.p2.y);
        block[i].l = find_slope_intercept(block[i].l);
    }

    scanf("%d",&num_mirrors);
    for(i=0; i<num_mirrors; i++)
    {
        scanf("%f%f%f%f",&mirror[i].l.p1.x,&mirror[i].l.p1.y,&mirror[i].l.p2.x,&mirror[i].l.p2.y);
        mirror[i].l = find_slope_intercept(mirror[i].l);
    }
    mouse.p.x = mouse.p.y = 0;
}

void save_world()
{
    int i;
    FILE *f = fopen("world.sav","w");
    fprintf(f,"%f %f\n",world.width,world.height);

    fprintf(f,"%d\n",num_projectors);
    for(i=0; i<num_projectors; i++)
    {
        fprintf(f,"%f %f %f %f\n",projector[i].l.p1.x,projector[i].l.p1.y,projector[i].l.p2.x,projector[i].l.p2.y);
        fprintf(f,"%f\n",projector[i].distance);
        fprintf(f,"%d\n",projector[i].num_pixels);
    }

    fprintf(f,"%d\n",num_blocks);
    for(i=0; i<num_blocks; i++)
        fprintf(f,"%f %f %f %f\n",block[i].l.p1.x,block[i].l.p1.y,block[i].l.p2.x,block[i].l.p2.y);

    fprintf(f,"%d\n",num_mirrors);
    for(i=0; i<num_mirrors; i++)
        fprintf(f,"%f %f %f %f\n",mirror[i].l.p1.x,mirror[i].l.p1.y,mirror[i].l.p2.x,mirror[i].l.p2.y);
}

void rotate_object(float rot_val,int side,int selected,int i)
{
    if(selected == PROJECTOR)
    {
        float x2,y2,c,m,d;
        m = tan(atan(projector[i].l.m) + rot_val);
        c = projector[i].l.p1.y - m*projector[i].l.p1.x;
        d = distance_between_points(projector[i].l.p1,projector[i].l.p2);
        x2 = projector[i].l.p1.x + sqrt( (d*d) / (1 + m*m));
        y2 = m*x2 + c;
        point p;
        p.x=x2,p.y=y2;
        if(find_side(projector[i].l.p1,projector[i].l.p2,p) == side)
        {
            x2 = projector[i].l.p1.x - sqrt( (d*d) / (1 + m*m));
            y2 = m*x2 + c;
        }
        if( abs(x2) >= world.width/2 || abs(y2) >= world.height/2 )
            return;
        projector[i].l.m = m;
        projector[i].l.c = c;
        projector[i].l.p2.x = x2;
        projector[i].l.p2.y = y2;
        projector[i].l = find_slope_intercept(projector[i].l);
        projector[i].d = find_source_point(projector[i]);
        divide_line(projector[i].l.p1,projector[i].l.p2,projector[i].num_pixels,i);
    }
    else if(selected == BLOCK)
    {
        float x2,y2,c,m,d;
        m = tan(atan(block[i].l.m) + rot_val);
        c = block[i].l.p1.y - m*block[i].l.p1.x;
        d = distance_between_points(block[i].l.p1,block[i].l.p2);
        x2 = block[i].l.p1.x + sqrt( (d*d) / (1 + m*m));
        y2 = m*x2 + c;
        point p;
        p.x=x2,p.y=y2;
        if(find_side(block[i].l.p1,block[i].l.p2,p) == side)
        {
            x2 = block[i].l.p1.x - sqrt( (d*d) / (1 + m*m));
            y2 = m*x2 + c;
        }
        if( abs(x2) >= world.width/2 || abs(y2) >= world.height/2 )
            return;
        block[i].l.m = m;
        block[i].l.c = c;
        block[i].l.p2.x = x2;
        block[i].l.p2.y = y2;
        block[i].l = find_slope_intercept(block[i].l);
    }
    else
    {
        float x2,y2,c,m,d;
        m = tan(atan(mirror[i].l.m) + rot_val);
        c = mirror[i].l.p1.y - m*mirror[i].l.p1.x;
        d = distance_between_points(mirror[i].l.p1,mirror[i].l.p2);
        x2 = mirror[i].l.p1.x + sqrt( (d*d) / (1 + m*m));
        y2 = m*x2 + c;
        point p;
        p.x=x2,p.y=y2;
        if(find_side(mirror[i].l.p1,mirror[i].l.p2,p) == side)
        {
            x2 = mirror[i].l.p1.x - sqrt( (d*d) / (1 + m*m));
            y2 = m*x2 + c;
        }
        if( abs(x2) >= world.width/2 || abs(y2) >= world.height/2 )
            return;
        mirror[i].l.m = m;
        mirror[i].l.c = c;
        mirror[i].l.p2.x = x2;
        mirror[i].l.p2.y = y2;
        mirror[i].l = find_slope_intercept(mirror[i].l);
    }
}

void select_object()
{
    if(select_type == PROJECTOR)
    {
        if(select_number >= num_projectors-1 || num_projectors == 0)
        {
            select_number = 0;
            select_type = BLOCK;
        }
        else
            select_number++;
    }
    else if(select_type == BLOCK)
    {
        if(select_number >= num_blocks-1 || num_blocks == 0)
        {
            select_number = 0;
            select_type = MIRROR;
        }
        else
            select_number++;
    }
    else
    {
        if(select_number >= num_mirrors-1 || num_mirrors == 0)
        {
            select_number = 0;
            select_type = PROJECTOR;
        }
        else
            select_number++;
    }
}

void delete_selected_object(int snum,int stype)
{
    int i;
    if(stype == PROJECTOR)
    {
        for(i=snum; i<num_projectors-1; i++)
            projector[i] = projector[i+1];
        num_projectors--;
    }
    else if(stype == MIRROR)
    {
        for(i=snum; i<num_mirrors-1; i++)
            mirror[i] = mirror[i+1];
        num_mirrors--;
    }
    else
    {
        for(i=snum; i<num_blocks-1; i++)
            block[i] = block[i+1];
        num_blocks--;
    }
    select_object();
}

void handle_keyboard_keys(unsigned char key ,int x ,int y)
{
    if(key == 27 || key == 'q')
    {
        save_world();
        exit(0);
    }
    if(key == 'z')
    {
        random_walk_mode = random_walk_mode?0:1;
    }
    if(key == 't')
    {
        light_transport_mode = light_transport_mode?0:1;
        update_timer = light_transport_mode?200:10;
    }
    if(key == 'g')
    {
        gaze_cursor_mode = gaze_cursor_mode?0:1;
    }
    if(key == 'o')
    {
        //Enter/Exit Play Game Mode
    }
    if(key == 'r')
    {
        rotate_object(-atan(rotat_val),-1,select_type,select_number);
    }
    if(key == 'l')
    {
        rotate_object(atan(rotat_val),1,select_type,select_number);
    }
    if(key == 'f')
    {
        trans_val += 0.2;
        rotat_val += 0.1;
    }
    if(key == 's')
    {
        trans_val -= 0.2;
        if(trans_val <= 0.0)
            trans_val = 0.2;
        rotat_val -= 0.1;
        if(rotat_val <= 0.0)
            rotat_val = 0.1;
    }
    if(key == 'b')
    {
        insert_object_mode = 1;
        insert_object_type = BLOCK;
    }
    if(key == 'm')
    {
        insert_object_mode = 1;
        insert_object_type = MIRROR;
    }
    if(key == 'p')
    {
        insert_object_mode = 1;
        insert_object_type = PROJECTOR;
    }
    if(key == 127)
    {
        delete_selected_object(select_number,select_type);
    }
    if(key == 'c')
    {
        select_object();
    }
}

void update_values(float x1,float y1,float x2,float y2,int selected,int i)
{
    if(selected == PROJECTOR)
    {
        if(abs(projector[i].l.p1.x+x1) >= world.width/2 || abs(projector[i].l.p2.x+x2) >= world.width/2
                || abs(projector[i].l.p1.y+y1) >= world.height/2 || abs(projector[i].l.p2.y+y2) >= world.height/2)
            return;
        projector[i].l.p1.x += x1;
        projector[i].l.p1.y += y1;
        projector[i].l.p2.x += x2;
        projector[i].l.p2.y += y2;
        projector[i].l = find_slope_intercept(projector[i].l);
        projector[i].d = find_source_point(projector[i]);
        divide_line(projector[i].l.p1,projector[i].l.p2,projector[i].num_pixels,i);
    }
    else if(selected == BLOCK)
    {
        if(abs(block[i].l.p1.x+x1) >= world.width/2 || abs(block[i].l.p2.x+x2) >= world.width/2
                || abs(block[i].l.p1.y+y1) >= world.height/2 || abs(block[i].l.p2.y+y2) >= world.height/2)
            return;
        block[i].l.p1.x += x1;
        block[i].l.p1.y += y1;
        block[i].l.p2.x += x2;
        block[i].l.p2.y += y2;
        block[i].l = find_slope_intercept(block[i].l);
    }
    else
    {
        if(abs(mirror[i].l.p1.x+x1) >= world.width/2 || abs(mirror[i].l.p2.x+x2) >= world.width/2
                || abs(mirror[i].l.p1.y+y1) >= world.height/2 || abs(mirror[i].l.p2.y+y2) >= world.height/2)
            return;
        mirror[i].l.p1.x += x1;
        mirror[i].l.p1.y += y1;
        mirror[i].l.p2.x += x2;
        mirror[i].l.p2.y += y2;
        mirror[i].l = find_slope_intercept(mirror[i].l);
    }
}

void handle_special_keyboard_keys(int key,int x,int y)
{
    if(key == GLUT_KEY_DOWN)
    {
        update_values(0,-trans_val,0,-trans_val,select_type,select_number);
    }
    if(key == GLUT_KEY_UP)
    {
        update_values(0,trans_val,0,trans_val,select_type,select_number);
    }
    if(key == GLUT_KEY_LEFT)
    {
        update_values(-trans_val,0,-trans_val,0,select_type,select_number);
    }
    if(key == GLUT_KEY_RIGHT)
    {
        update_values(trans_val,0,trans_val,0,select_type,select_number);
    }
}

void generate_colors()
{
    srand(time(NULL));
    int i;
    for(i=0; i<MAX*5; i++)
    {
        color[i].r = rand_num();
        color[i].g = rand_num();
        color[i].b = rand_num();
    }
}

void update_random_value(int type,int type_num,int motion_type)
{
    float random_walk_value = 0.1;
    float random_rotate_value = 0.025;
    switch(motion_type)
    {
    case 0:
        update_values(0,-random_walk_value,0,-random_walk_value,type,type_num);
        break;
    case 1:
        update_values(0,random_walk_value,0,random_walk_value,type,type_num);
        break;
    case 2:
        update_values(-random_walk_value,0,-random_walk_value,0,type,type_num);
        break;
    case 3:
        update_values(random_walk_value,0,random_walk_value,0,type,type_num);
        break;
    case 4:
        rotate_object(-atan(random_rotate_value),-1,type,type_num);
        break;
    case 5:
        rotate_object(atan(random_rotate_value),1,type,type_num);
        break;
    }
    glutPostRedisplay();
}

void random_walk(int value)
{
    if(!random_walk_mode)
    {
        glutTimerFunc(100,random_walk,0);
        return;
    }
    int i;
    for(i=0; i<num_projectors; i++)
        update_random_value(PROJECTOR,i,rand()%6);
    for(i=0; i<num_mirrors; i++)
        update_random_value(MIRROR,i,rand()%6);
    for(i=0; i<num_blocks; i++)
        update_random_value(BLOCK,i,rand()%6);
    glutTimerFunc(100,random_walk,0);
}

int check_quadrant(point p)
{
    if(p.x > 0 && p.y > 0)
        return 1;
    else if(p.x < 0 && p.y > 0)
        return 2;
    else if(p.x < 0 && p.y < 0)
        return 3;
    else
        return 4;
}

void gaze_cursor(int value)
{
    if(!gaze_cursor_mode)
    {
        glutTimerFunc(10,gaze_cursor,0);
        return;
    }
    int i = 0;
    for(i=0; i<num_projectors; i++)
    {
        point center;
        center.x = (projector[i].l.p1.x + projector[i].l.p2.x)/2;
        center.y = (projector[i].l.p1.y + projector[i].l.p2.y)/2;
        float m = tan(atan2(center.y - mouse.p.y , center.x - mouse.p.x));
        float m1 = -1.0/m;
        float x3 = center.x;
        float y3 = center.y;
        float l = distance_between_points(projector[i].l.p1,projector[i].l.p2);
        float val = sqrt((l*l)/(4+4*m1*m1));
        float c1 = y3 - m1*x3;
        float x2 = x3 + val;
        float x1 = 2*x3 - x2;
        float y1 = x1*m1 + c1;
        float y2 = x2*m1 + c1;
        projector[i].l.p1.x = x1;
        projector[i].l.p1.y = y1;
        projector[i].l.p2.x = x2;
        projector[i].l.p2.y = y2;
        if(find_side(projector[i].l.p1,projector[i].l.p2,projector[i].d) == -1)
        {
            float x2 = x3 - val;
            float x1 = 2*x3 - x2;
            float y1 = x1*m1 + c1;
            float y2 = x2*m1 + c1;
            projector[i].l.p1.x = x1;
            projector[i].l.p1.y = y1;
            projector[i].l.p2.x = x2;
            projector[i].l.p2.y = y2;
        }
        projector[i].l.m = m1;
        projector[i].l.c = c1;
        //projector[i].l = find_slope_intercept(projector[i].l);
        projector[i].d = find_source_point(projector[i]);
        divide_line(projector[i].l.p1,projector[i].l.p2,projector[i].num_pixels,i);
    }
    glutTimerFunc(10,gaze_cursor,0);

}

void get_mouse_coordinates(int x,int y)
{
    int viewport[4];
    double modelview[16];
    double projection[16];
    float winX, winY, winZ;
    double posX, posY, posZ;
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_COMPONENT);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    winX = (float)x;
    winY = (float)viewport[3] - (float)y;
    glReadPixels( int(winX), int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    mouse.p.x = posX;
    mouse.p.y = posY;
}

void mouse_action(int x, int y)
{
    get_mouse_coordinates(x,y);
}

void select_nearest_object()
{
    float dist = INF;
    int i,stype,snum;
    for(i=0; i<num_projectors; i++)
    {
        float d = distance_line_point(projector[i].l,mouse.p);
        if(d<dist)
        {
            stype = PROJECTOR;
            snum = i;
            dist = d;
        }
    }
    for(i=0; i<num_mirrors; i++)
    {
        float d = distance_line_point(mirror[i].l,mouse.p);
        if(d<dist)
        {
            stype = MIRROR;
            snum = i;
            dist = d;
        }
    }
    for(i=0; i<num_blocks; i++)
    {
        float d = distance_line_point(block[i].l,mouse.p);
        if(d<dist)
        {
            stype = BLOCK;
            snum = i;
            dist = d;
        }
    }
    select_number = snum;
    select_type = stype;
}

int dragging = 0,drag_modifier;

void start_draw(point p,int modifier)
{
    dragging = 1;
    drag_modifier = modifier;
    if(insert_object_type == MIRROR)
    {
        mirror[num_mirrors].l.p1 = p;
        mirror[num_mirrors].l.p2 = p;
        num_mirrors++;
    }
    else if(insert_object_type == BLOCK)
    {
        block[num_blocks].l.p1 = p;
        block[num_blocks].l.p2 = p;
        num_blocks++;
    }
    else
    {
        projector[num_projectors].l.p1 = p;
        projector[num_projectors].l.p2 = p;
        projector[num_projectors].distance = 5.0;
        projector[num_projectors].num_pixels = 20;
        num_projectors++;
    }
}

void finish_draw(point p)
{
    if(!dragging)
        return;
    dragging = 0;
    if(insert_object_type == MIRROR)
        mirror[num_mirrors-1].l = find_slope_intercept(mirror[num_mirrors-1].l);
    else if(insert_object_type == BLOCK)
        block[num_blocks-1].l = find_slope_intercept(block[num_blocks-1].l);
    else
    {
        int i = num_projectors - 1;
        projector[i].l = find_slope_intercept(projector[i].l);
        projector[i].d = find_source_point(projector[i]);
        divide_line(projector[i].l.p1,projector[i].l.p2,projector[i].num_pixels,i);
    }
    insert_object_mode = 0;
    glutPostRedisplay();
}

void continue_draw(point p)
{
    if(!dragging)
        return;
    bool shifted = ((drag_modifier & GLUT_ACTIVE_SHIFT) != 0);
    if(insert_object_type == MIRROR)
    {
        int i = num_mirrors-1;
        if(shifted)
        {
            if (abs(p.x - mirror[i].l.p1.x) > abs(p.y - mirror[i].l.p1.y))
                p.y = mirror[i].l.p1.y;
            else
                p.x = mirror[i].l.p1.x;
        }
        mirror[i].l.p2 = p;
    }
    else if(insert_object_type == BLOCK)
    {
        int i = num_blocks-1;
        if(shifted)
        {
            if (abs(p.x - block[i].l.p1.x) > abs(p.y - block[i].l.p1.y))
                p.y = block[i].l.p1.y;
            else
                p.x = block[i].l.p1.x;
        }
        block[i].l.p2 = p;
    }
    else
    {
        int i = num_projectors-1;
        if(shifted)
        {
            if (abs(p.x - projector[i].l.p1.x) > abs(p.y - projector[i].l.p1.y))
                p.y = projector[i].l.p1.y;
            else
                p.x = projector[i].l.p1.x;
        }
        projector[i].l.p2 = p;
    }
    glutPostRedisplay();
}

void click_action(int button, int state,int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        get_mouse_coordinates(x,y);
        select_nearest_object();
    }
    if(insert_object_mode)
        if(button == GLUT_LEFT_BUTTON)
        {
            get_mouse_coordinates(x,y);
            if(state == GLUT_DOWN)
                start_draw(mouse.p,glutGetModifiers());
            else
                finish_draw(mouse.p);
        }
}

void click_hold_action(int x,int y)
{
    if(dragging)
    {
        get_mouse_coordinates(x,y);
        continue_draw(mouse.p);
    }
}

int main(int argc ,char **argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA);
    FILE *f = freopen(argv[1],"r",stdin);
    scanner();
    generate_colors();
    w = glutGet(GLUT_SCREEN_WIDTH);
    h = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitWindowPosition(100,100);
    glutInitWindowSize(w*3/4,h*3/4);
    glutCreateWindow("Graphics.... :)");

    initRendering();
    glutDisplayFunc(draw_scene);
    glutReshapeFunc(reshape_window);
    glutKeyboardFunc(handle_keyboard_keys);
    glutSpecialFunc(handle_special_keyboard_keys);
    glutMouseFunc(click_action);
    glutMotionFunc(click_hold_action);
    glutPassiveMotionFunc(mouse_action);
    //glutPostRedisplay();
    glutTimerFunc(100,random_walk,0);
    glutTimerFunc(10,gaze_cursor,0);
    glutTimerFunc(update_timer,light_transport,0);
    //glutIdleFunc(draw_scene);
    glutMainLoop();
    return 0;
}
