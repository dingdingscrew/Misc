# -*- coding: utf-8 -*- 
import os
import sys
import xlrd 
import xlwt 
from datetime import date,datetime 

reload(sys)
sys.setdefaultencoding('utf8')

def read_excel(): 
	workbook = xlrd.open_workbook(r'./Aquila_Registers.xls')	
	#print workbook.sheet_names() 
	#sheet2_name = workbook.sheet_names()[1]
	#print sheet2_name
	#sheet2 = workbook.sheet_by_index(1) # sheet索引从0开始  
	#print sheet2 
	sheet2 = workbook.sheet_by_name("RTC") 

	print sheet2.name,sheet2.nrows,sheet2.ncols 
	
	rows = sheet2.row_values(19) # 获取第四行内容 
	#cols = sheet2.col_values(2) # 获取第三列内容

	print rows   
	#print cols 
	
	print sheet2.cell(20,0).value.encode('utf-8') 
	print sheet2.cell_value(20,0).encode('utf-8')   
	print sheet2.row(21)[0].value.encode('utf-8')       

	# 获取单元格内容的数据类型   
	print sheet2.cell(20,0).ctype   

if __name__ == '__main__':   
	read_excel()
