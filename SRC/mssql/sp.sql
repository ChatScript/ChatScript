-- Routines to set up stored procedures

USE chatscript;  
GO  

IF OBJECT_ID('uspGetUserData', 'P') IS NOT NULL  
    DROP PROCEDURE uspGetUserData;  
GO  
CREATE PROCEDURE uspGetUserData (
       @userid varchar(100)
)
AS    
    SELECT [file]  
    FROM userfiles
    WHERE userid = @userid;  
RETURN;
GO

IF OBJECT_ID('uspSetUserData', 'P') IS NOT NULL  
    DROP PROCEDURE uspSetUserData;  
GO  
CREATE PROCEDURE uspSetUserData (
    @userid AS VARCHAR(100),
    @userdata AS VARBINARY(max)
)
AS    
BEGIN
    MERGE INTO userfiles AS t
      USING 
        (SELECT pk=@userid, f1= @userdata) AS s
      ON t.userid = s.pk
      WHEN MATCHED THEN
        UPDATE SET userid=s.pk, [file]=s.f1
      WHEN NOT MATCHED THEN
        INSERT (userid, [file])
        VALUES (s.pk, s.f1);
END;
RETURN  
GO  

