
    
pass1:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass1_next 
    SW X2 0 X0     
    SW X1 1 X0    
pass1_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 pass1_next2
    SW X2 1 X0     
    SW X1 2 X0    
pass1_next2:
    LD X1 2 X0     
    LD X2 3 X0     
    BLE X1 X2 pass1_next3
    SW X2 2 X0     
    SW X1 3 X0    
pass1_next3:
    LD X1 3 X0     
    LD X2 4 X0     
    BLE X1 X2 pass1_next4
    SW X2 3 X0     
    SW X1 4 X0    
pass1_next4:
    LD X1 4 X0     
    LD X2 5 X0     
    BLE X1 X2 pass1_next5
    SW X2 4 X0     
    SW X1 5 X0    
pass1_next5:
    LD X1 5 X0     
    LD X2 6 X0     
    BLE X1 X2 pass2
    SW X2 5 X0     
    SW X1 6 X0    
    J pass2

    
pass2:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass2_next
    SW X2 0 X0     
    SW X1 1 X0    
pass2_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 pass2_next2
    SW X2 1 X0     
    SW X1 2 X0    
pass2_next2:
    LD X1 2 X0     
    LD X2 3 X0     
    BLE X1 X2 pass2_next3
    SW X2 2 X0     
    SW X1 3 X0    
pass2_next3:
    LD X1 3 X0     
    LD X2 4 X0     
    BLE X1 X2 pass2_next4
    SW X2 3 X0     
    SW X1 4 X0    
pass2_next4:
    LD X1 4 X0     
    LD X2 5 X0     
    BLE X1 X2 pass2_next5
    SW X2 4 X0     
    SW X1 5 X0    
pass2_next5:
    LD X1 5 X0     
    LD X2 6 X0     
    BLE X1 X2 pass3
    SW X2 5 X0     
    SW X1 6 X0    
    J pass3

   
pass3:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass3_next
    SW X2 0 X0     
    SW X1 1 X0    
pass3_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 pass3_next2
    SW X2 1 X0     
    SW X1 2 X0    
pass3_next2:
    LD X1 2 X0     
    LD X2 3 X0     
    BLE X1 X2 pass3_next3
    SW X2 2 X0     
    SW X1 3 X0    
pass3_next3:
    LD X1 3 X0     
    LD X2 4 X0     
    BLE X1 X2 pass3_next4
    SW X2 3 X0     
    SW X1 4 X0    
pass3_next4:
    LD X1 4 X0     
    LD X2 5 X0     
    BLE X1 X2 pass4
    SW X2 4 X0     
    SW X1 5 X0    
    J pass4

 
pass4:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass4_next
    SW X2 0 X0     
    SW X1 1 X0    
pass4_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 pass4_next2
    SW X2 1 X0     
    SW X1 2 X0    
pass4_next2:
    LD X1 2 X0     
    LD X2 3 X0     
    BLE X1 X2 pass4_next3
    SW X2 2 X0     
    SW X1 3 X0    
pass4_next3:
    LD X1 3 X0     
    LD X2 4 X0     
    BLE X1 X2 pass5
    SW X2 3 X0     
    SW X1 4 X0    
    J pass5

    
pass5:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass5_next
    SW X2 0 X0     
    SW X1 1 X0    
pass5_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 pass5_next2
    SW X2 1 X0     
    SW X1 2 X0    
pass5_next2:
    LD X1 2 X0     
    LD X2 3 X0     
    BLE X1 X2 pass6
    SW X2 2 X0     
    SW X1 3 X0    
    J pass6

    
pass6:
    LD X1 0 X0     
    LD X2 1 X0     
    BLE X1 X2 pass6_next
    SW X2 0 X0     
    SW X1 1 X0    
pass6_next:
    LD X1 1 X0     
    LD X2 2 X0     
    BLE X1 X2 end
    SW X2 1 X0     
    SW X1 2 X0    

end:
    J end        