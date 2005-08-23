#include <rsf.h>
/*^*/

#include "hwt3d.h"

#include "vecop.h"
/*^*/

#define MISSING 0
/*^*/

static axa az,ax,ay;
static axa at,ag,ah;


/*------------------------------------------------------------*/
void hwt3d_init(axa      az_in    /* z axis   */,
		axa      ax_in    /* x axis   */,
		axa      ay_in    /* y axis   */,
		axa      at_in    /* t axis   */,
		axa      ag_in    /* g axis   */,
		axa      ah_in    /* h axis   */)
/*< initialize hwt3d >*/
{
    az = az_in;
    ax = ax_in;
    ay = ay_in;

    at = at_in;
    ag = ag_in;
    ah = ah_in;
}

void hwt3d_rand(axa      az_in    /* z axis   */,
		axa      ax_in    /* x axis   */,
		axa      ay_in    /* y axis   */,
		axa      at_in    /* t axis   */)
/*< initialize hwt3d (reduced # of variables) >*/
{
    az = az_in;
    ax = ax_in;
    ay = ay_in;

    at = at_in;
}

/*------------------------------------------------------------*/
float hwt3d_getv(float ***vv,
		 pt3d P) 
/*< get velocity from 3-D cube by linear interpolation >*/
{
    double  z, x, y;
    double rz,rx,ry;
    int    jz,jx,jy;
    double fz,fx,fy;
    float  v;

    x = SF_MIN(SF_MAX(ax.o,P.x),ax.o+(ax.n-2)*ax.d);
    y = SF_MIN(SF_MAX(ay.o,P.y),ay.o+(ay.n-2)*ay.d);
    z = SF_MIN(SF_MAX(az.o,P.z),az.o+(az.n-2)*az.d);

    rx = (x-ax.o)/ax.d;
    jx = (int)(rx);
    fx =       rx-jx;

    ry = (y-ay.o)/ay.d;
    jy = (int)(ry);
    fy =       ry-jy;

    rz = (z-az.o)/az.d;
    jz = (int)(rz);
    fz =       rz-jz;

    v = vv[ jy  ][ jx  ][ jz  ] * (1-fz)*(1-fx)*(1-fy) +
	vv[ jy  ][ jx  ][ jz+1] * (  fz)*(1-fx)*(1-fy) +
	vv[ jy+1][ jx  ][ jz  ] * (1-fz)*(1-fx)*(  fy) +
	vv[ jy+1][ jx  ][ jz+1] * (  fz)*(1-fx)*(  fy) + 
	vv[ jy  ][ jx+1][ jz  ] * (1-fz)*(  fx)*(1-fy) +
	vv[ jy  ][ jx+1][ jz+1] * (  fz)*(  fx)*(1-fy) +
	vv[ jy+1][ jx+1][ jz  ] * (1-fz)*(  fx)*(  fy) +
	vv[ jy+1][ jx+1][ jz+1] * (  fz)*(  fx)*(  fy);

    return(v);
}

/*------------------------------------------------------------*/

bool hwt3d_cusp(pt3d Tm, /* it-1,ig   ,ih   */
		pt3d To, /* it  ,ig   ,ih   */
		pt3d Gm, /* it  ,ig-1 ,ih   */
		pt3d Gp, /* it  ,ig+1 ,ih   */
		pt3d Hm, /* it  ,ig   ,ih-1 */
		pt3d Hp) /* it  ,ig   ,ih+1 */
/*< find cusp points >*/
{
    int sGmHm, sGmHp;
    int sGpHm, sGpHp;
    int sHmGm, sHmGp;
    int sHpGm, sHpGp;
    bool c;
    
    sGmHm = SF_SIG( jac3d(&To,&Tm,&Gm,&Hm) );
    sGmHp = SF_SIG( jac3d(&To,&Tm,&Gm,&Hp) );

    sGpHm = SF_SIG( jac3d(&To,&Tm,&Gp,&Hm) );
    sGpHp = SF_SIG( jac3d(&To,&Tm,&Gp,&Hp) );

    sHmGm = SF_SIG( jac3d(&To,&Tm,&Hm,&Gm) );
    sHmGp = SF_SIG( jac3d(&To,&Tm,&Hm,&Gp) );

    sHpGm = SF_SIG( jac3d(&To,&Tm,&Hp,&Gm) );
    sHpGp = SF_SIG( jac3d(&To,&Tm,&Hp,&Gp) );

    c = sGmHm*sGmHp==1 ||
	sGpHm*sGpHp==1 ||
	sHmGm*sHmGp==1 ||
	sHpGm*sHpGp==1;

    if(c)
	return true;
    else
	return false;
}

/*------------------------------------------------------------*/

pt3d hwt3d_wfttr(float ***vv,
		 pt3d Tm, 
		 pt3d To,
		 pt3d Gm, 
		 pt3d Gp,
		 pt3d Hm,
		 pt3d Hp)
/*< wavefront tracing >*/
{
    pt3d Tp;

    /* execute HWT step 
     * from Tm & (Gm,Hm,To,Gp,Hp) to Tp
     */
    Tp=hwt3d_step(vv,Tm,To,Gm,Gp,Hm,Hp);
    return(Tp);
}

/*------------------------------------------------------------*/

pt3d hwt3d_raytr(float ***vv,
		 pt3d Tm, 
		 pt3d To,
		 int scaleray)
/*< ray tracing >*/
{
    pt3d Tp;
    pt3d Gm,Gp,Hm,Hp;
    vc3d TmTo;

    double a1,a2,a3;
    vc3d   v1,v2,v3;
    vc3d   qq,uu,ww; /* unit vectors */
    float ro;

    ro = To.v * at.d; /* ray step */
    TmTo = vec3d(&Tm,&To);       /* ray vector */

    /* axes unit vectors */
    v1 = axa3d(1);
    v2 = axa3d(2);
    v3 = axa3d(3);

    /* ray angle with axes*/
    a1 = ang3d(&TmTo, &v1); a1 = SF_ABS(a1);
    a2 = ang3d(&TmTo, &v2); a2 = SF_ABS(a2);
    a3 = ang3d(&TmTo, &v3); a3 = SF_ABS(a3);

    /* select reference unit vector 
       as "most orthogonal" to in-comming ray 
     */
    if(      SF_ABS(a1-90) <= SF_ABS(a2-90) &&
	     SF_ABS(a1-90) <= SF_ABS(a3-90) )
	ww=v1;
    else if( SF_ABS(a2-90) <= SF_ABS(a1-90) &&
	     SF_ABS(a2-90) <= SF_ABS(a3-90) )
	ww=v2;
    else 
	ww=v3;

    /* build orthogonal wavefront (Gm,Gp,Hm,Hp) */
    qq   = scl3d(&ww,+1);
    uu   = vcp3d(&TmTo,&qq);       /* uu = TmTo x qq */
    qq   = nor3d(&uu);             /* qq = uu / |uu| */
    uu   = scl3d(&qq,ro*scaleray); /* uu = qq * ro */
    Gm   = tip3d(&To,&uu);         /* Gm at tip of uu from To */
    Gm.v = hwt3d_getv(vv,Gm);

    qq   = scl3d(&ww,-1);
    uu   = vcp3d(&TmTo,&qq); 
    qq   = nor3d(&uu);
    uu   = scl3d(&qq,ro*scaleray);
    Gp   = tip3d(&To,&uu);
    Gp.v = hwt3d_getv(vv,Gp);

    uu = vec3d(&Gm,&Gp);
    ww = nor3d(&uu);

    qq   = scl3d(&ww,+1);
    uu   = vcp3d(&TmTo,&qq); 
    qq   = nor3d(&uu);
    uu   = scl3d(&qq,ro*scaleray);
    Hm   = tip3d(&To,&uu);
    Hm.v = hwt3d_getv(vv,Hm);

    qq   = scl3d(&ww,-1);
    uu   = vcp3d(&TmTo,&qq); 
    qq   = nor3d(&uu);
    uu   = scl3d(&qq,ro*scaleray);
    Hp   = tip3d(&To,&uu);
    Hp.v = hwt3d_getv(vv,Hp);

    /* execute HWT step 
     * from Tm & (Gm,Hm,To,Gp,Hp) to Tp
     */
    Tp=hwt3d_step(vv,Tm,To,Gm,Gp,Hm,Hp);
    return(Tp);
}

/*------------------------------------------------------------*/

pt3d hwt3d_step(float ***vv,
		pt3d Tm, 
		pt3d To, 
		pt3d Gm, 
		pt3d Gp,
		pt3d Hm,
		pt3d Hp)
/*< one HWT time step >*/
{
    double gdx,gdy,gdz,gdr;
    double hdx,hdy,hdz,hdr;
    double ddx,ddy,ddz;
    double ax,bx,ay,by,az,bz;
    double a,b,c,del;

    double dx,dy,dz;

    pt3d Sm,Sp,S;
    vc3d TmTo,ToSm,ToSp;
    double am,ap;
    float ro;

/*------------------------------------------------------------*/

    ro = To.v * at.d;       /* ray step */

    gdx = Gp.x-Gm.x;
    gdy = Gp.y-Gm.y;
    gdz = Gp.z-Gm.z;
    gdr =(Gp.v-Gm.v)*at.d;

    hdx = Hp.x-Hm.x;
    hdy = Hp.y-Hm.y;
    hdz = Hp.z-Hm.z;
    hdr =(Hp.v-Hm.v)*at.d;

    /* find largest dd (avoid division by 0) */
    ddz = gdy*hdx - gdx*hdy;
    ddx = gdz*hdy - gdy*hdz;
    ddy = gdx*hdz - gdz*hdx;

    if( SF_ABS(ddz) >=SF_ABS(ddx) && 
	SF_ABS(ddz) > SF_ABS(ddy)) {

	ax  = gdr*hdy - hdr*gdy; 
	bx  = gdy*hdz - gdz*hdy; 
	ay  = gdr*hdx - gdx*hdr; 
	by  = gdx*hdz - gdz*hdx; 
	
	a = ddz*ddz + bx*bx + by*by;
	b =         -(ax*bx + ay*by);           b *= ro;
	c =           ax*ax + ay*ay - ddz*ddz;  c *= ro*ro;
	
	del = SF_MAX(b*b - a*c , 0.);
	del = sqrtf(del);
		
	dz = (-b+del)/a;  Sp.z = To.z + dz;
	dx = ro*ax-dz*bx; Sp.x = To.x + dx/  ddz;
	dy = ro*ay-dz*by; Sp.y = To.y + dy/(-ddz);
	
	dz = (-b-del)/a;  Sm.z = To.z + dz;
	dx = ro*ax-dz*bx; Sm.x = To.x + dx/  ddz;
	dy = ro*ay-dz*by; Sm.y = To.y + dy/(-ddz);
	
    } else if( SF_ABS(ddx) >=SF_ABS(ddy) &&
	       SF_ABS(ddx) > SF_ABS(ddz)) {

	ay  = gdr*hdz - hdr*gdz; 
	by  = gdz*hdx - gdx*hdz; 
	az  = gdr*hdy - gdy*hdr; 
	bz  = gdy*hdx - gdx*hdy; 

	a = ddx*ddx + by*by + bz*bz;
	b =         -(ay*by + az*bz);           b *= ro;
	c =           ay*ay + az*az - ddx*ddx;  c *= ro*ro;
	
	del = SF_MAX(b*b - a*c , 0.);
	del = sqrtf(del);

	dx = (-b+del)/a;  Sp.x = To.x + dx;
	dy = ro*ay-dx*by; Sp.y = To.y + dy/  ddx;
	dz = ro*az-dx*bz; Sp.z = To.z + dz/(-ddx);
	
	dx = (-b-del)/a;  Sm.x = To.x + dx;
	dy = ro*ay-dx*by; Sm.y = To.y + dy/  ddx;
	dz = ro*az-dx*bz; Sm.z = To.z + dz/(-ddx);

    } else {

	az  = gdr*hdx - hdr*gdx; 
	bz  = gdx*hdy - gdy*hdx; 
	ax  = gdr*hdz - gdz*hdr; 
	bx  = gdz*hdy - gdy*hdz; 

	a = ddy*ddy + bz*bz + bx*bx;
	b =         -(az*bz + ax*bx);           b *= ro;
	c =           az*az + ax*ax - ddy*ddy;  c *= ro*ro;
	
	del = SF_MAX(b*b - a*c , 0.);
	del = sqrtf(del);

	dy = (-b+del)/a;  Sp.y = To.y + dy;
	dz = ro*az-dy*bz; Sp.z = To.z + dz/  ddy;
	dx = ro*ax-dy*bx; Sp.x = To.x + dx/(-ddy);
	
	dy = (-b-del)/a;  Sm.y = To.y + dy;
	dz = ro*az-dy*bz; Sm.z = To.z + dz/  ddy;
	dx = ro*ax-dy*bx; Sm.x = To.x + dx/(-ddy);
    }

    TmTo = vec3d( &Tm, &To); /* in-comming ray */
    ToSm = vec3d( &To, &Sm); /*  candidate ray */
    ToSp = vec3d( &To, &Sp); /*  candidate ray */
    
    /* angle between ray segments */
    am = ang3d( &TmTo, &ToSm);
    ap = ang3d( &TmTo, &ToSp);

    /* select candidate point that moves forward */
    if(am<ap) S=Sm;
    else      S=Sp;
    S.v = hwt3d_getv(vv,S);

    return S;
}

/*------------------------------------------------------------*/
void hwt3d_putt(float*** tt, /* traveltime cube */
		float*** ll, /*     length cube */
		pt3d     P,  /* coordinates (x,y,z) */
		float    t,  /*   time from source to P */
		float    l)  /* length from source to P */
/*< interpolate traveltime >*/
{
    double rz,rx,ry;
    int    jz,jx,jy;
    double fz,fx,fy;

    rx = (P.x-ax.o)/ax.d;
    jx = (int)(rx);
    fx =       rx-jx;

    ry = (P.y-ay.o)/ay.d;
    jy = (int)(ry);
    fy =       ry-jy;
    
    rz = (P.z-az.o)/az.d;
    jz = (int)(rz);
    fz =       rz-jz;
    
    if( 0<=jx && jx<ax.n-1 &&
	0<=jy && jy<ay.n-1 &&
	0<=jz && jz<az.n-1 ) {

/*	sf_warning("jx=%d jy=%d jz=%d t=%g l=%g",jx,jy,jz,t,l);*/

/*	sf_warning("jx=%d jy=%d jz=%d",jx,jy,jz);*/
/*	sf_warning("fx=%g fy=%g fz=%g",fx,fy,fz);*/

	if(false) {

	    fz=0.5;
	    fy=0;
	    fx=0.5;

	    tt[ jy  ][ jx  ][ jz  ] = t * (1-fz)*(1-fx)*(1-fy);
	    tt[ jy  ][ jx  ][ jz+1] = t * (  fz)*(1-fx)*(1-fy);
	    tt[ jy+1][ jx  ][ jz  ] = t * (1-fz)*(1-fx)*(  fy);
	    tt[ jy+1][ jx  ][ jz+1] = t * (  fz)*(1-fx)*(  fy); 
	    tt[ jy  ][ jx+1][ jz  ] = t * (1-fz)*(  fx)*(1-fy);
	    tt[ jy  ][ jx+1][ jz+1] = t * (  fz)*(  fx)*(1-fy);
	    tt[ jy+1][ jx+1][ jz  ] = t * (1-fz)*(  fx)*(  fy);
	    tt[ jy+1][ jx+1][ jz+1] = t * (  fz)*(  fx)*(  fy);

	} else {
	    
	    tt[ jy  ][ jx  ][ jz  ] = t;
	    tt[ jy  ][ jx  ][ jz+1] = t;
	    tt[ jy+1][ jx  ][ jz  ] = t;
	    tt[ jy+1][ jx  ][ jz+1] = t;
	    tt[ jy  ][ jx+1][ jz  ] = t;
	    tt[ jy  ][ jx+1][ jz+1] = t;
	    tt[ jy+1][ jx+1][ jz  ] = t;
	    tt[ jy+1][ jx+1][ jz+1] = t;
	}
    }
}

/*------------------------------------------------------------*/
void lpick(float*** tt, float*** ll,
	  float     t, float     l,
	  int iy,int ix,int iz) {

    if( ll[iy][ix][iz] == MISSING || 
	ll[iy][ix][iz] >=l ) {
	tt[iy][ix][iz]= t;
	ll[iy][ix][iz]= l;
    }
}

void hwt3d_lint(float*** tt, /* traveltime cube */
		float*** ll, /*     length cube */
		pt3d     P,  /* coordinates (x,y,z) */
		float    t,  /*   time from source to P */
		float    l)  /* length from source to P */
/*< interpolate traveltime >*/
{
    double rz,rx,ry;
    int    jz,jx,jy;

    rx = (P.x-ax.o)/ax.d;
    jx = (int)(rx);

    ry = (P.y-ay.o)/ay.d;
    jy = (int)(ry);
    
    rz = (P.z-az.o)/az.d;
    jz = (int)(rz);
    
    if( 0<=jx && jx<ax.n-1 &&
	0<=jy && jy<ay.n-1 &&
	0<=jz && jz<az.n-1 ) {
	
	lpick(tt,ll,t,l,jy  ,jx  ,jz  );
	lpick(tt,ll,t,l,jy  ,jx  ,jz+1);
	lpick(tt,ll,t,l,jy+1,jx  ,jz  );
	lpick(tt,ll,t,l,jy+1,jx  ,jz+1);
	lpick(tt,ll,t,l,jy  ,jx+1,jz  );
	lpick(tt,ll,t,l,jy  ,jx+1,jz+1);
	lpick(tt,ll,t,l,jy+1,jx+1,jz  );
	lpick(tt,ll,t,l,jy+1,jx+1,jz+1);
    }
}

/*------------------------------------------------------------*/

void tpick(float*** tt, float*** ll,
	  float     t, float     l,
	  int iy,int ix,int iz) {
    if( tt[iy][ix][iz]== MISSING ||
	tt[iy][ix][iz]>=t )
	tt[iy][ix][iz]= t;
}

void hwt3d_tint(float*** tt, /* traveltime cube */
		float*** ll, /*     length cube */
		pt3d     P,  /* coordinates (x,y,z) */
		float    t,  /*   time from source to P */
		float    l)  /* length from source to P */
/*< interpolate traveltime >*/
{
    double rz,rx,ry;
    int    jz,jx,jy;

    rx = (P.x-ax.o)/ax.d;
    jx = (int)(rx);

    ry = (P.y-ay.o)/ay.d;
    jy = (int)(ry);
    
    rz = (P.z-az.o)/az.d;
    jz = (int)(rz);
    
    if( 0<=jx && jx<ax.n-1 &&
	0<=jy && jy<ay.n-1 &&
	0<=jz && jz<az.n-1 ) {
	
	tpick(tt,ll,t,l,jy  ,jx  ,jz  );
	tpick(tt,ll,t,l,jy  ,jx  ,jz+1);
	tpick(tt,ll,t,l,jy+1,jx  ,jz  );
	tpick(tt,ll,t,l,jy+1,jx  ,jz+1);
	tpick(tt,ll,t,l,jy  ,jx+1,jz  );
	tpick(tt,ll,t,l,jy  ,jx+1,jz+1);
	tpick(tt,ll,t,l,jy+1,jx+1,jz  );
	tpick(tt,ll,t,l,jy+1,jx+1,jz+1);
    }
}

/*------------------------------------------------------------*/

void hwt3d_nint(float*** tt, /* traveltime cube */
		float*** ll, /*     length cube */
		pt3d     P,  /* coordinates (x,y,z) */
		float    t,  /*   time from source to P */
		float    l)  /* length from source to P */
/*< interpolate traveltime >*/
{
    double rz,rx,ry;
    int    jz,jx,jy;

    rx = (P.x-ax.o)/ax.d;
    jx = (int)(rx);

    ry = (P.y-ay.o)/ay.d;
    jy = (int)(ry);
    
    rz = (P.z-az.o)/az.d;
    jz = (int)(rz);
    
    if( 0<=jx && jx<ax.n-1 &&
	0<=jy && jy<ay.n-1 &&
	0<=jz && jz<az.n-1 ) {
	
	tt[ jy  ][ jx  ][ jz  ] = t;
	tt[ jy  ][ jx  ][ jz+1] = t;
	tt[ jy+1][ jx  ][ jz  ] = t;
	tt[ jy+1][ jx  ][ jz+1] = t;
	tt[ jy  ][ jx+1][ jz  ] = t;
	tt[ jy  ][ jx+1][ jz+1] = t;
	tt[ jy+1][ jx+1][ jz  ] = t;
	tt[ jy+1][ jx+1][ jz+1] = t;
    }
}


/*------------------------------------------------------------*/

void hwt3d_fill(float*** tt, /* traveltime cube */
		int n)
/*< fill holes >*/
{
    int ix,iy,iz;
    int kx,ky,kz;
    int lx,ly,lz;
    int jx,jy,jz;
    float v;
    int   k;

    for(iy=0;iy<ay.n;iy++) {
	for(ix=0;ix<ax.n;ix++) {
	    for(iz=0;iz<az.n;iz++) {

		//---------------------------------------
		if( tt[iy][ix][iz] == MISSING) {

		    kx=ix-n; lx=ix+n;
		    ky=iy-n; ly=iy+n;
		    kz=iz-n; lz=iz+n;

		    k=0;
		    v=0.;
		    for(jy=ky;jy<ly;jy++) {
			for(jx=kx;jx<lx;jx++) {
			    for(jz=kz;jz<lz;jz++) {

				if(jy>0 && jy<ay.n &&
				   jx>0 && jx<ax.n &&
				   jz>0 && jz<az.n )
				    if( tt[jy][jx][jz] != MISSING) {
					v+=tt[jy][jx][jz];
					k++;
				    }
			    }
			}
		    } // local loop
		    
		    if(k>0) tt[iy][ix][iz] = v/k;
		    
		} // end if MISSING
		//---------------------------------------

	    }
	}
    } // global loop
}
