#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.

	// LAB 4: Your code here.

    	// SUNUS,DEC 25    	
	static int old_envid = 0;
	static int circular;
    	int next_envid = old_envid + 1;
	//cprintf("cur_envid = %d,next_envid = %d\n",cur_envid,next_envid);
	circular = NENV - 1;
	while(circular--) // it's ok to see if curenv is runnable
	  {
	    if(envs[next_envid].env_status == ENV_RUNNABLE)
	      {
		//cprintf("env %d begins to run\n",next_envid);
		old_envid = next_envid;
		env_run(&envs[next_envid]);
		return ;
	      }
	    next_envid++;
	    if(next_envid == NENV - 1)
		next_envid = 1;
	  }
	/*
      	int i, tmp_env;
        static int old_env = 0;
        
        for (i=1; i<=NENV; i++) {
                tmp_env = (i + old_env) % NENV;
                if (tmp_env && envs[tmp_env].env_status == ENV_RUNNABLE) {
                       // dbg_print("old=%d, new=%d, 0x%08x => 0x%08x", old_env, tmp_env, envs[old_env].env_id, envs[tmp_env].env_id);
                        old_env = tmp_env;
                        env_run(&envs[tmp_env]);
                        return;
                }
        }
	*/
	// Run the special idle environment when nothing else is runnable.
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
